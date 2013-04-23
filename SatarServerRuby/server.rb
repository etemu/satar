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

require 'erb'	 # view
require 'sinatra'# controller
require 'yaml'
require 'rubygems'
require 'dm-core'
require 'dm-redis-adapter'
require 'dm-migrations'

### load the config
config = YAML.load_file("./_config/config.yml")

### DataMapper models
DataMapper.setup(:default, {:adapter  => "redis"})

class Node
	include DataMapper::Resource

	property :id,		Integer, :required => true, :key => true
	property :delta,	Integer
	property :status,	Integer
end

class Rider
	include DataMapper::Resource

	property :id,			Integer, :required => true, :key => true
	property :lastTime,		Integer
	has n, :results
end

class Result
	include DataMapper::Resource

	property :id,		Integer, :required => true, :key => true
	property :time,		Integer
	belongs_to :Rider,  		:key => true
end

class Event
	include DataMapper::Resource

	property :id,		Serial, :key => true
	property :time,		Integer
end

DataMapper.finalize
DataMapper.auto_upgrade!

### sinatra
set :port, config["server"]["port"]
set :bind, config["server"]["bind"]
set :server, 'thin'

### variables for streaming
$connectionsDebug = [] # Debuglog
$connectionsEvent = [] # Trigger events
$connectionsSystem = [] # System status
$connectionsResults = [] # pairs of trigger events

### routes
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
	erb :admin
end

### API
post '/api/event' do
	timestamp = params['TSN'].to_i
	eventId = params['EVENT'].to_i
	nodeId = params['NODE'].to_i
	statusId = params['ID'].to_i
	debug "(#{nodeId}) E#{eventId}\n\n"
	case eventId
		### status/bootup
		when 0
			debug "(#{nodeId}) booted up"
			node = Node.new(:id => nodeId)
			debug "#{node.id}"
			node.delta = (Time.now.to_f * 1000).floor - timestamp
			node.save
		### status/keepalive
		when 1
			node = Node.get(nodeId)
			offsetNew = (Time.now.to_f * 1000).floor - timestamp
			# fliter out peaks
			if node.delta > 0
				if (node.delta-offsetNew).abs > 250
					offsetNew = node.delta
				else
					offsetNew = (offsetNew+node.delta)/2
				end
			end
			debug "(#{nodeId}) has an offset of #{offsetNew}"
			node.status = statusId
			node.save

		### eventId >= 100: hardware event!
		when 100..108
		### log it
			node = Node.get(nodeId)
			debug "(#{nodeId}) triggered input #{eventId-100}"
			if node.delta!=nil
				relativeTime = timestamp+node.delta
				event = Event.new 
				event.time = relativeTime
				event.save
				### stream it so that a riderId can be connected
				timestring = Time.at(relativeTime.to_f/1000).strftime("%H:%M:%S,%L")
				$connectionsEvent.each { |out| out <<
					"data: #{nodeId};#{timestring};#{event.id}\n\n"}
			end
			node.save
		### other stuff
		else
			debug "unknown eventID"
	end
	updateSystemStatus
	204 # response without entity body
end
	
### connect event /w rider
post '/api/event/:nodeID/:eventKey' do
	riderId = params['riderId']
	eventKey = params[:eventKey]
	event = Event.get(eventKey)
	rider = Rider.get(riderId)
	debug "#{rider.lastTime}"
	debug "connected event #{eventKey}/#{params[:nodeID]} with rider #{riderId}"
	if rider.lastTime.nil?
		debug "was nil"
		rider.lastTime = event.time
		debug "#{rider.lastTime}"
		debug "#{rider.save}"
		debug "#{rider.lastTime}"
	else
		diff = (lastTime - event.time).abs
		result = Result.new(:time => diff)
		rider.results << result
		rider.lastTime.destroy
		debug "data: got a time of #{diff}ms for rider #{riderId}"
		$connectionsResults.each { |out| out <<
			"data: #{riderId};#{diff}\n\n"}
	end
	rider.save
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

### helpers

helpers do
	### system status
	def updateSystemStatus
		base = "<h2>Nodes</h2><ul>"
		for node in Node.all do
			base << "<li>#{node.id.to_s.rjust(3,"0")}:"
			base << "st #{node.status.to_i.to_s(2).rjust(3,"0")} "
			base << "of #{node.delta}</li>"
		end
	 	base += "</ul>"
		$connectionsSystem.each { |out| out << "data: #{base}\n\n"}
	end
	def debug(string)
		$connectionsDebug.each { |out| out <<
		"data: #{ts}: #{string}\n\n"}
	end
	def ts
		Time.now.strftime("%H:%M:%S,%L")
	end
end