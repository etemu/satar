#!/usr/bin/env ruby -I ../lib -I lib
# coding: utf-8
################################################################################
# SatarServer Ruby by Leon Rische
# 0.1.0
# 	+ calculating offsets and relative times for nodes
# 	+ session storage for the node selector
# 	+ now deleting a riders timeset after completion
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
# variables for streaming
# 
$connectionsDebug = [] # Debuglog
$connectionsEvent = [] # Trigger events
$connectionsSystem = [] # System status
$connectionsResults = [] # pairs of trigger events

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
	timestamp = params['TSN'].to_i
	eventId = params['EVENT'].to_i
	nodeId = params['NODE']
	statusId = params['ID'].to_i
	$connectionsDebug.each { |out| out <<
		"data: #{ts}: N#{nodeId}, E#{eventId}\n\n"}
	case eventId
		### status/bootup
		when 0
			$connectionsDebug.each { |out| out <<
				"data: #{ts}: (#{nodeId}) booted up\n\n"}
			addNode(nodeId)
		### status/keepalive
		when 1
			addNode(nodeId)
			offsetNew = (Time.now.to_f * 1000).floor - timestamp
			offsetOld = $redis.hget("node:#{nodeId}",'offset').to_i
			# fliter out peaks
			if offsetOld > 0
				if (offsetOld-offsetNew).abs > 0.1
					offsetNew = offsetOld
				else
					offsetNew = (offsetNew+offsetOld)/2
				end
			end
			$connectionsDebug.each { |out| out <<
				"data: #{ts}: (#{nodeId}) has an offset of #{offsetNew}\n\n"}
			$redis.hmset("node:#{nodeId}",'status',statusId,'offset',offsetNew)
			if $redis.hget("node:#{nodeId}",'status').to_i != statusId
				$connectionsDebug.each { |out| out <<
					"data: #{ts}: (#{nodeId}) changed status to #{statusId.to_s(2)}\n\n"}
			end
		### eventId >= 100: hardware event!
		when 100..108
		### log it
			$connectionsDebug.each { |out| out <<
				"data: #{ts}: N#{nodeId} triggered input #{eventId-100}\n\n"}
			offset = $redis.hget("node:#{nodeId}",'offset').to_i
			if offset!=nil
				relativeTime = timestamp+offset
				$redis.incr("event.id")
				eventKey = $redis.get("event.id")
				$redis.sadd("events:#{nodeId}", eventKey)
				$redis.set("event:#{eventKey}",relativeTime)
		
				### stream it so that a riderId can be connected
				timestring = Time.at(relativeTime.to_f/1000).strftime("%H:%M:%S,%L")
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
	riderId = params['riderId']
	eventKey = params[:eventKey]
	$connectionsDebug.each { |out| out <<
		"data: #{ts}: connected event #{eventKey}/#{params[:nodeID]} with rider #{riderId}\n\n"}
	eventKey2 = $redis.get("time:#{riderId}")
	if eventKey2 == nil
		$redis.set("time:#{riderId}",eventKey)
		$connectionsDebug.each { |out| out <<
			"data: no matching pair right now\n\n"}
	else
		time1 = $redis.get("event:#{eventKey2}")
		time2 = $redis.get("event:#{eventKey}")
		diff = (time2.to_i - time1.to_i).abs
		$connectionsDebug.each { |out| out <<
			"data: got a time of #{diff}ms for rider #{riderId}\n\n"}
		$connectionsResults.each { |out| out <<
			"data: #{riderId};#{diff}\n\n"}
		$redis.del("time:#{riderId}")
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
			base << "of #{node['offset']}</li>"
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
	def ts
		Time.now.strftime("%H:%M:%S,%L")
	end

	### db helpers
	def addNode(nodeId)
		if $redis.sadd('nodes', nodeId) == true
			# this is obviously completely unnecessary and stupid
			# but makes it easier so iterate trough all nodes later
			$redis.hset("node:#{nodeId}",'id',nodeId)
			$connectionsDebug.each { |out| out <<
				"data: #{ts}: Added new node (#{nodeId})\n\n"}
		end
	end
end