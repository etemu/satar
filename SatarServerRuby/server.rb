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
config = YAML.load_file('./_config/config.yml')

### DataMapper models
DataMapper.setup(:default, {:adapter  => 'redis', 
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
set :port, config['server']['port']
set :bind, config['server']['bind']
set :server, 'thin'

### variables for streaming
$connections_debug = [] # Debuglog
$connections_system = [] # System status
$connections_results = [] # pairs of trigger events

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
	stream_debug "#{@results.count}"
	erb :'raw/result', :layout => false
end

get '/raw/events/:nodeID' do
	node_ID = params[:nodeID]
	# just get the last 10 events
	@node = Node.get(node_ID)
	if @node.nil?
		'Unknown node'
	else
		erb :'raw/event', :layout => false
	end
end

### API
post '/api/event' do
	timestamp = params['TSN'].to_i
	event_ID = params['EVENT'].to_i
	node_ID = params['NODE'].to_i
	status_ID = params['ID'].to_i
	case event_ID
		### status/bootup
		when 0
			stream_debug "(#{node_ID}) booted up"
			node = Node.new(:id => node_ID)
			node.delta = millis - timestamp
			node.save
			stream_system 'status'
		### status/keepalive
		when 1
			node = Node.get(node_ID)
			offsetNew = millis - timestamp
			node.var = offsetNew - node.delta
			# fliter out peaks
			if node.delta.abs > 0 && (node.delta-offsetNew).abs < 5
				node.delta = (offsetNew+node.delta)/2
			end
			stream_debug "(#{node_ID}) has an offset of #{node.delta}"
			node.status = status_ID
			node.save
			stream_system 'status'
		### event_ID >= 100: hardware event!
		when 100..108
		### log it
			node = Node.get(node_ID)
			stream_debug "(#{node_ID}) triggered input #{event_ID-100}"
			if node.delta!=nil
				relativeTime = timestamp+node.delta
				event = Event.create(:time => relativeTime)
				### stream it so that a riderId can be connected
				node.events << event
				node.save # prevent fuckup when accessing the last event
				stream_system "event;#{node.id}"
			end
			node.save
		### other stuff
		else
			stream_debug 'unknown eventID'
	end
	204 # response without entity body
end
	
### connect event /w rider
post '/api/event/:node_ID/:eventKey' do
	rider_ID = params['riderId'].to_i
	event_key = params[:eventKey].to_i
	node_ID = params[:nodeId].to_i
	event = Event.get(event_key)
	rider = Rider.get(rider_ID)
	if rider.nil?
		rider = Rider.new(:id => rider_ID)
	end
	stream_debug "connected #{event_key}/#{node_ID} with rider #{rider_ID}"
	if rider.last.nil?
		rider.last = event.time
	else
		diff = (rider.last - event.time).abs
		result = Result.create(:time => diff)
		rider.results << result
		rider.last = nil
		stream_debug "got a time of #{diff}ms for rider #{rider_ID}"
		stream_result "#{rider_ID};#{diff}"
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
		$connections_debug << out
		out.callback { $connections_debug.delete(out) }
	end
end
get '/api/stream/system', :provides => 'text/event-stream' do
	stream :keep_open do |out|
		$connections_system << out
		out.callback { $connections_system.delete(out) }
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
		$connections_results << out
		out.callback { $connections_results.delete(out) }
	end
end

### helpers
helpers do
	### system status
	def stream_debug(string)
		$connections_debug.each { |out| out <<
		"data: #{ts}: #{string}\n\n"}
	end
	def stream_result(string)
		$connections_results.each { |out| out <<
		"data: #{ts}: #{string}\n\n"}
	end
	def stream_system(string)		
		$connections_system.each { |out| out <<
		"data: #{string}\n\n"}
	end
	def ts
		Time.now.strftime('%H:%M:%S,%L')
	end
	def millis
		(Time.now.to_f * 1000).floor
	end
end

