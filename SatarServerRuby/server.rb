#!/usr/bin/env ruby -I ../lib -I lib
# coding: utf-8
################################################################################
# SatarServer Ruby by Leon Rische
# 0.2.0
# 	+ changed the way events are streamed
# 		+ the node list is now loaded when opening the page
# 		+ the event list for each node is safed and reloaded dynamically
# 		+ improvend the stream/node selection function (reload on change)
# 		+ using one stream for all change events
# 		+ checked in events are deleted now
# 	+ moved all html creation to erb files
# 	+ displaying a nodes var in the interface
# 	+ calculating deltas between nodes
# 0.1.1
# 	+ using data mapper now
# 	+ one rider can have many results
# 0.1.0
# 	+ calculating offsets and relative times for nodes
# 	+ session storage for the node selector
# 	+ now deleting a riders timeset after completion
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
DataMapper.setup(:default, {:adapter  => "redis", 
							:host => '127.0.0.1', 
							:port => '6379'})

class Node
	include DataMapper::Resource

	property :id,		Integer, :required => true, :key => true
	property :delta,	Integer
	property :var,		Integer	
	property :status,	Integer
	has n, :events
end

class Event
	include DataMapper::Resource

	property :id,		Serial, :key => true
	property :time,		Integer
	belongs_to :node
end

class Rider
	include DataMapper::Resource

	property :id,			Integer, :required => true, :key => true
	property :last,			Integer
	# not yet implemented
	# property :first_name,	String
	# property :last_name,	String
	# property :team,		String
	has n, :results
end

class Result
	include DataMapper::Resource

	property :id,		Integer, :required => true, :key => true
	property :time,		Integer
	belongs_to :rider,  		:key => true
end

DataMapper.finalize
DataMapper.auto_upgrade!

Rider.all.destroy
Node.all.destroy
Event.all.destroy
Result.all.destroy

### sinatra
set :port, config["server"]["port"]
set :bind, config["server"]["bind"]
set :server, 'thin'

### variables for streaming
$connectionsDebug = [] # Debuglog
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

### raw views
get '/raw/nodes' do
	@nodes = Node.all
	erb :'raw/nodes', :layout => false
end

get '/raw/result' do
	@results = Result.all
	streamDebug "#{@results.count}"
	erb :'raw/result', :layout => false
end

get '/raw/events/:nodeID' do
	id = params[:nodeID]
	# just get the last 10 events
	@node = Node.get(id)
	if @node.nil?
		"Unknown node"
	else
		erb :'raw/event', :layout => false
	end
end

### API
post '/api/event' do
	timestamp = params['TSN'].to_i
	eventId = params['EVENT'].to_i
	nodeId = params['NODE'].to_i
	statusId = params['ID'].to_i
	case eventId
		### status/bootup
		when 0
			streamDebug "(#{nodeId}) booted up"
			node = Node.new(:id => nodeId)
			node.delta = millis - timestamp
			node.save
			streamSystem "status"
		### status/keepalive
		when 1
			node = Node.get(nodeId)
			offsetNew = millis - timestamp
			node.var = offsetNew - node.delta
			# fliter out peaks
			if node.delta.abs > 0
				if (node.delta-offsetNew).abs < 5
					node.delta = (offsetNew+node.delta)/2
				end
			end
			streamDebug "(#{nodeId}) has an offset of #{node.delta}"
			node.status = statusId
			node.save
			streamSystem "status"
		### eventId >= 100: hardware event!
		when 100..108
		### log it
			node = Node.get(nodeId)
			streamDebug "(#{nodeId}) triggered input #{eventId-100}"
			if node.delta!=nil
				relativeTime = timestamp+node.delta
				event = Event.create(:time => relativeTime)
				### stream it so that a riderId can be connected
				node.events << event
				node.save # prevent fuckup when accessing the last event
				streamSystem "hello?"
				streamSystem "event;#{node.id}"
			end
			node.save
		### other stuff
		else
			streamDebug "unknown eventID"
	end
	204 # response without entity body
end
	
### connect event /w rider
post '/api/event/:nodeId/:eventKey' do
	riderId = params['riderId'].to_i
	eventKey = params[:eventKey].to_i
	nodeId = params[:nodeId].to_i
	event = Event.get(eventKey)
	rider = Rider.get(riderId)
	if rider.nil?
		rider = Rider.new(:id => riderId)
	end
	streamDebug "connected #{eventKey}/#{nodeId} with rider #{riderId}"
	if rider.last.nil?
		rider.last = event.time
	else
		diff = (rider.last - event.time).abs
		result = Result.create(:time => diff)
		rider.results << result
		rider.last = nil
		streamDebug "got a time of #{diff}ms for rider #{riderId}"
		streamResult "#{riderId};#{diff}"
	end
	rider.save
	event.destroy
	204
end

post '/admin/reset' do
	# the redis adapter has no flush function so we gotta destroy them all
	Rider.all.destroy
	Node.all.destroy
	Event.all.destroy
	Result.all.destroy
	erb :admin
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
	def streamDebug(string)
		$connectionsDebug.each { |out| out <<
		"data: #{ts}: #{string}\n\n"}
	end
	def streamResult(string)
		$connectionsResults.each { |out| out <<
		"data: #{ts}: #{string}\n\n"}
	end
	def streamSystem(string)		
		$connectionsSystem.each { |out| out <<
		"data: #{string}\n\n"}
	end
	def ts
		Time.now.strftime("%H:%M:%S,%L")
	end
	def millis
		(Time.now.to_f * 1000).floor
	end
end