#!/usr/bin/env ruby -I ../lib -I lib
# coding: utf-8
################################################################################
# SatarServerRuby by Leon Rische
# 
# TODO: make sinatra an object
# 		remove global vars
# 
# 0.2.2
# 	+ option to start and stop a race run via the admin menu
# 0.2.1
# 	+ MySQL database output of race results
# 0.2.0
# 	+ changed the way events are streamed
# 		+ the node list is now loaded when opening the page for the first time
# 		+ the event list for each node is safed and reloaded dynamically
# 		+ improvend the stream/node selection function (reload on change)
# 		+ using one stream for all change events
# 		+ events which are checked in are deleted now
# 	+ moved all html creation to erb files
# 	+ displaying a node's var in the interface
# 	+ calculating deltas between nodes
# 0.1.1
# 	+ using data mapper now
# 	+ one rider can have many results
# 0.1.0
# 	+ calculating offsets and relative times for nodes
# 	+ session storage for the node selector
# 	+ now deleting a riders timeset after completion
################################################################################

require 'rubygems'
require 'erb'	 # view
require 'sinatra'# controller
require 'yaml'
require 'mysql'
require 'dm-core'
require 'dm-redis-adapter'
require 'dm-migrations'

# setup
################################################################################

# load the config
$config = YAML.load_file(File.expand_path File.dirname(__FILE__) + '/config/config.yml')

# DataMapper models
if $config['redis']['mode'] == 0
	DataMapper.setup(:default, {:adapter  => 'redis', 
								:host => $config['redis']['host'],
								:port => $config['redis']['port']
								})
else
	DataMapper.setup(:default, {:adapter  => 'redis', 
								:path => $config['redis']['socket']
								})
end

# DataMapper classes
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

# sinatra setup
set :port, $config['server']['port']
set :bind, $config['server']['bind']
set :server, 'thin' # supports event-machine

# variables for streaming
$connections_debug = [] # Debuglog
$connections_system = [] # System status
$connections_results = [] # pairs of trigger events

$race_id = 0

# routes
################################################################################

# main views
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

# raw views
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
	@node = Node.get(node_ID)
	if @node.nil?
		'Select a node below.'
	else
		erb :'raw/event', :layout => false
	end
end

# API
post '/api/event' do
	# extract the parameters out of the post
	timestamp = params['TSN'].to_i
	event_ID = params['EVENT'].to_i
	node_ID = params['NODE'].to_i
	status_ID = params['ID'].to_i
	
	if Node.get(node_ID).nil? # check if the node_ID is virgin or if it already occured at least once.
		node = Node.create(:id => node_ID, :delta => (millis - timestamp), :status => 1337, :lastPacket => millis) # node.delta = delta to server time
		else
		node = Node.get(node_ID)
	end

	case event_ID
		when 0 # status/bootup
			stream_debug "[<b>#{node_ID}</b>], boot up in #{timestamp} ms."
		when 1 # status/keepalive
			offsetNew = millis - timestamp # calculate the new offset
			node.var = offsetNew - node.delta # compare it to the last known offset
			# smooth the offset, a crude fliter:
			if (node.delta.abs > 0) && (node.var.abs < 128) # if the deviation is below 128 ms, use the mean average.
				node.delta = (offsetNew+node.delta)/2
				stream_debug "[<b>#{node_ID}</b>] &Delta; drift: #{node.var}"
				else
					node.delta=offsetNew # set the new offset node <-> server without filtering
					stream_debug "[<b>#{node_ID}</b>] reset &Delta; to server: #{node.delta} (&Delta;': #{node.var})"
			end
			node.status = status_ID
			node.save
			send_log(node_ID,node.delta,node.var)
		when 100..108 # event_ID >= 100: hardware event!
			stream_debug "[<b>#{node_ID}</b>] triggered input #{event_ID-100}"
			if node.delta!=nil
				relativeTime = timestamp+node.delta
				event = Event.create(:time => relativeTime)
				# stream it so that a riderId can be connected
				node.events << event
				node.save # prevent fuckup when accessing the last event
				stream_system "event;#{node.id}"
			end
			node.save
		else
			stream_debug '[<b>#{node_ID}</b>] sent unknown eventID #{event_ID}'
	end
	stream_system 'status' # update the status display
	204 # response without entity body
end
	
# connect event /w rider
post '/api/event/:node_ID/:eventKey' do
	# extract the parameters
	rider_ID = params['riderId'].to_i
	event_key = params[:eventKey].to_i
	node_ID = params[:nodeId].to_i # is this really the corresponding node ID ?
	
	# get the corresponding rider and event
	event = Event.get(event_key)
	rider = Rider.get(rider_ID)

	# create a new rider if it was his first event
	if rider.nil?
		rider = Rider.new(:id => rider_ID)
	end
	
	stream_debug "Connected event [<b>#{node_ID}</b>]/#{event_key} with rider #{rider_ID}" # TODO, DEBUG: node_ID seems to be 0 all the time
	
	if rider.last.nil? # first of 2 events
		rider.last = event.time
	else # second of 2 events
		diff = (rider.last - event.time).abs
		result = Result.create(:time => diff)
		rider.results << result
		# reset
		rider.last = nil
		stream_debug "Run time of #{diff}ms for rider #{rider_ID}"
		stream_result "#{rider_ID};#{diff}"
		# log to the sql db
	 	if $race_id > 0
			send_result($race_id, rider_ID, diff) unless $config['debug'] == '1'
		end
	end
	rider.save
	event.destroy
	204
end

post '/admin/reset' do
	# the redis adapter has no flush function so we need destroy them ‚by hand‘
	Rider.all.destroy
	Node.all.destroy
	Event.all.destroy
	Result.all.destroy
	erb :admin
	204
end

post '/admin/run/start' do
	$race_id = params['race_id'].to_i
	stream_debug "Race run <b>#{$race_id}</b> started."
	204
end

post '/admin/run/stop' do
	stream_debug "Race run <b>#{$race_id}</b> stopped."
	$race_id = 0
	204
end

# streaming
################################################################################

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

# helpers
################################################################################

helpers do
	# streaming
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

	# time
	def ts
		Time.now.strftime('%H:%M:%S,%L')
	end
	def millis
		(Time.now.to_f * 1000).floor
	end
	
	# MySQL logging of the results and for debugging purposes
	def send_result(raceID, userID, res)
		begin
			con = Mysql.new($config['sql']['host'],
							$config['sql']['user'],
							$config['sql']['pw'],
							$config['sql']['db'])
			con.query "INSERT INTO #{$config['sql']['table']} (#{$config['sql']['raceID']},#{$config['sql']['userID']},#{$config['sql']['runtime']}) VALUES (#{raceID},#{userID},#{res})" 
		rescue Mysql::Error => e
			stream_debug "SQL reply: #{e.errno}#{e.error}"
		ensure
			con.close if con
		end
	end
	
	def send_log(nodeID, nodeDelta, nodeDev)
		begin
			con = Mysql.new($config['sql_log']['host'],
							$config['sql_log']['user'],
							$config['sql_log']['pw'],
							$config['sql_log']['db'])
			con.query "INSERT INTO #{$config['sql_log']['table']} (#{$config['sql_log']['nodeID']},#{$config['sql_log']['nodeDelta']},#{$config['sql_log']['nodeDev']}) VALUES (#{nodeID},#{nodeDelta},#{nodeDev})" 
		rescue Mysql::Error => e
			stream_debug "SQL log reply: #{e.errno}#{e.error}"
		ensure
			con.close if con
		end
	end
end

