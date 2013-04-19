#!/usr/bin/env ruby -I ../lib -I lib
# coding: utf-8
################################################################################
# SatarServer Ruby by Leon Rische
# 0.0.9
#   + form for stream events
# 0.0.8b
# 	+ config file
# 0.0.8a
# 	+ database stuff
# 	+ system status updating
################################################################################
#  ___      __    __                 
# /\_ \   /'__`\ /\ \                
# \//\ \ /\_\L\ \\ \ \/'\     ___    
#   \ \ \\/_/_\_<_\ \ , <   /' _ `\  
#    \_\ \_/\ \L\ \\ \ \\`\ /\ \/\ \ 
#    /\____\ \____/ \ \_\ \_\ \_\ \_\
#    \/____/\/___/   \/_/\/_/\/_/\/_/

require 'redis'	 # model
require 'erb'	 # view 
require 'sinatra'# controller
require 'yaml'


# load the config
config = YAML.load_file("./_config/config.yml")

################################################################################
# setup

### redis
if config['redis']['mode'] == 0
	$redis = Redis.new(:path => config["redis"]["socket"])
else
	$redis = Redis.new(:host => config["redis"]["host"],
					   :port => config["redis"]["port"])
end

$redis.select(config["redis"]["number"])
$redis.flushdb

### sinatra
set :port, config["server"]["port"]
set :bind, config["server"]["bind"]
set :server, 'thin'

################################################################################
# variables

# streaming
$connectionsDebug = [] # Debuglog
$connectionsEvent = [] # Trigger events
$connectionsSystem = [] # System status 
$connectionsResults = [] # pairs of trigger events

$trigger = 0

################################################################################
# routes

### main views
get '/' do
	erb :index
end

get '/stream' do
	erb :stream
end

get '/results' do
	erb :results
end

get '/admin' do
	protected!
	erb :admin
end

### API
post '/api/event' do
	timestamp = params['TSN'].to_f
	eventId = params['EVENT'].to_i
	nodeId = params['NODE'].to_i
	statusId = params['ID'].to_i

	$connectionsDebug.each { |out| out << 
		"data: #{Time.now.to_i}: N#{nodeId}, E#{eventId}\n\n"}

	case eventId
	### status/bootup
		when 0 
			$connectionsDebug.each { |out| out << 
				"data: #{Time.now.to_i}: (#{nodeId}) booted up\n\n"}
			addNode(nodeId)

	### status/keepalive
		when 1
			addNode(nodeId)
			offsetNew = Time.now.to_f - timestamp
			offsetOld = $redis.hget("node:#{nodeId}",'offset').to_f
			# fliter out peaks
			if offsetOld > 0
				if (offsetOld-offsetNew).abs > 0.1
					offsetNew = offsetOld
				else
					offsetNew = (offsetNew+offsetOld)/2
				end
			end
			$connectionsDebug.each { |out| out << 
				"data: #{Time.now.to_i}: (#{nodeId}) has an offset of #{offsetNew}\n\n"}	
			$redis.hmset("node:#{nodeId}",'status',statusId,'offset',offsetNew)
			if $redis.hget("node:#{nodeId}",'status').to_i != statusId
				$connectionsDebug.each { |out| out << 
					"data: #{Time.now.to_i}: (#{nodeId}) changed status to #{statusId.to_s(2)}\n\n"}	
			end

	### eventId >= 100: hardware event!
		when 100..108
		### log it
			$connectionsDebug.each { |out| out << 
				"data: #{Time.now.to_i}: N#{nodeId} triggered input #{eventId-100}\n\n"}

		### 1. save it /w an unique id in a set
		### 2. set its hash to its timestamp
		### 3. send it so the stream /w an unique id
			offset = $redis.hget("node:#{nodeId}",'offset').to_f
			if offset!=nil
				relativeTime = timestamp+offset
				$redis.incr("event.id")
				eventKey = $redis.get("event.id")
				$redis.sadd("events:#{nodeId}", eventKey)
				$redis.set("event:#{eventKey}",relativeTime)

			
				### stream it so that a riderId can be connected
				timestring = Time.at(relativeTime).strftime("%H:%M:%S,%L")
				$connectionsEvent.each { |out| out << 
					"data: #{nodeId};#{timestring};#{eventKey}\n\n"}
			end
	### other stuff
		else
			$connectionsDebug.each { |out| out << 
				"data: unknown eventID\n\n"}
	end
	updateSystemStatus
	204 # response without entity body
end

	### connect event /w rider
post '/api/event/:nodeID/:eventKey' do
	riderId = params['riderId'].to_i
	eventKey = params[:eventKey]
	$connectionsDebug.each { |out| out << 
		"data: #{Time.now.to_i}: connected event #{eventKey}/#{params[:nodeID]} with rider #{riderId}\n\n"}
	eventKey2 = $redis.get("time:#{riderId}")
	if eventKey2 == nil
		$redis.set("time:#{riderId}",eventKey)
		$connectionsDebug.each { |out| out << 
			"data: no matching pair right now\n\n"}
	else
		time1 = $redis.get("event:#{eventKey2}")
		time2 = $redis.get("event:#{eventKey}")
		diff = (time2.to_f - time1.to_f).abs
		$connectionsDebug.each { |out| out << 
			"data: got a time of #{diff.round(4)} for rider #{riderId}\n\n"}
		$connectionsResults.each { |out| out << 
			"data: #{riderId};#{diff.round(4)}\n\n"}
	end

	204
end
	
### streaming
get '/api/stream/debug', :provides => 'text/event-stream' do
	stream :keep_open do |out|
		$connectionsDebug << out
		out.callback { $connectionsDebug.delete(out) }
	end
end

get '/api/stream/system', :provides => 'text/event-stream' do
	stream :keep_open do |out|
		$connectionsSystem << out
		out.callback { $connectionsSystem.delete(out) }
	end
end

get '/api/stream/events', :provides => 'text/event-stream' do
	stream :keep_open do |out|
		$connectionsEvent << out
		out.callback { $connectionsEvent.delete(out) }
	end
end

get '/api/stream/results', :provides => 'text/event-stream' do
	stream :keep_open do |out|
		$connectionsResults << out
		out.callback { $connectionsResults.delete(out) }
	end
end

################################################################################
# helpers

helpers do
	
### authentication 
	def protected!
		unless authorized?
			response['WWW-Authenticate'] = %(Basic realm="Restricted Area")
			throw(:halt, [401, "Not authorized\n"])
		end
	end
	def authorized?
		@auth ||=	Rack::Auth::Basic::Request.new(request.env)
		@auth.provided? && @auth.basic? && @auth.credentials == ['admin', 'admin']
	end

### system status
	def updateSystemStatus
		base = "<h2>Nodes</h2><ul>"
		for node in allNodes do
			base << "<li>#{node['id'].to_s.rjust(3,"0")}:" 
			base << "st #{node['status'].to_i.to_s(2).rjust(3,"0")} "
			base << "of #{node['offset'].to_f.round(4)}</li>"
		end
	 	base += "</ul>"

		$connectionsSystem.each { |out| out << "data: #{base}\n\n"}
	end

	def allNodes
	 	node_ids = $redis.smembers("nodes")
	 	nodes = $redis.pipelined {
	 		node_ids.each{|id|
	 			$redis.hgetall("node:#{id}")
	 		}
	 	}
	 	return nodes
	end

### db helpers
	def addNode(nodeId)
		if $redis.sadd('nodes', nodeId) == true
			# this is obviously completely unnecessary and stupid
			# but makes it easier so iterate trough all nodes later
			$redis.hset("node:#{nodeId}",'id',nodeId)
			$connectionsDebug.each { |out| out << 
				"data: #{Time.now.to_i}: Added new node (#{nodeId})\n\n"}
		end
	end
end

################################################################################
# protocol
### params
	
# TSN - timestamp
# EVENT - event ID
# NODE - node ID
# ID - rider ID

### packets

# event = 0 id = foo => bootup
# event = 1 id = 0..255 => status
### 0 0 0 0 0 0 0 0 -> unarmed
### 0 0 0 0 0 0 1 1  -> in 1 & 2 armed
# event = 100 id = foo => trigger

# database
# nodes => set /w all nodes
# node:id => hashes, status, offset
# time:riderId
# 