#!/usr/bin/env ruby -I ../lib -I lib
# coding: utf-8
################################################################################
# SatarServerRuby by Leon Rische
################################################################################

require 'erb'	 # view
require 'sinatra'# controller
require 'json'
require 'yaml'
require 'mysql'
require 'dm-core'
require 'dm-redis-adapter'
require 'dm-migrations'
require 'dm-serializer'

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
    property :node_id, Integer # we can not access the rider from knockout.js
	property :time,		Integer
	belongs_to :node
end

class Debug
	include DataMapper::Resource
	property :id,		Serial, :key => true
	property :time,		Integer
	property :text,		String
end

class Rider
	include DataMapper::Resource
	property :id,			Integer, :required => true, :key => true
	property :last,			Integer
	has n, :results
end

class Result
	include DataMapper::Resource
	property :id,		Serial, :key => true
	property :time,		Integer
    property :rider_id, Integer # we can not access the rider from knockout.js
	belongs_to :rider,  		:key => true
end

configure do 
	DataMapper.finalize
	DataMapper.auto_upgrade!

	# sinatra setup
	set :port, $config['server']['port']
	set :bind, $config['server']['bind']
	set :server, 'thin' # supports event-machine
end

# variables for streaming
$connections_debug  = [] # Debuglog
$connections_result = [] # pairs of trigger events
$connections_event  = []
$connections_node   = []

$race_id = 0

# routes
################################################################################

# prefix
before '/json/*' do
	content_type 'application/json'
end

# main views
get '/'        do erb :index   end
get '/stream'  do erb :stream  end
get '/results' do erb :results end
get '/admin'   do erb :admin   end

# raw api
get '/raw/debug'   do erb :'raw/debug', :layout => false end
get '/raw/event'  do erb :'raw/event', :layout => false end
get '/raw/event/:id'  do 
    erb :'raw/event_single', :layout => false, :locals => {:id => params[:id].to_i}
end
get '/raw/result' do erb :'raw/result', :layout => false end

# json

get '/json/event' do
    Event.all.to_json
end

get '/json/node' do
    Node.all.to_json
end

get '/json/result' do
    Result.all.to_json
end

get '/json/debug' do
    Debug.all.take(10).to_json
end

get '/json/riders' do
    Riders.all.take(10).to_json
end



# API
post '/api/event' do
	# extract the parameters out of the post
	timestamp = params['TSN'].to_i
	event_ID = params['EVENT'].to_i
	node_ID = params['NODE'].to_i
	status_ID = params['ID'].to_i
    
	case event_ID
		when 0 # status/bootup
			node = Node.create(:id => node_ID, :delta => millis - timestamp, :status => 1337) # node.delta = delta to server time=
            stream_node node.to_json
        	
		when 1 # status/keepalive
			node = Node.get(node_ID)
        	if node.nil?
                node = Node.create(:id => node_ID, :delta => millis - timestamp, :status => status_ID)
                stream_debug "(#{node_ID}, #{timestamp}:#{millis}) create on keepalive"
            else
                offsetNew = millis - timestamp # calculate the new offset
                node.var = offsetNew - node.delta # compare it to the last known offset
                # smooth the offset, a crude fliter
    
                if (node.delta.abs > 0) && (node.var.abs < 100) # (todo: comment this line)
                    node.delta = (offsetNew+node.delta) / 2 
                    stream_debug "(#{node_ID}) momentary delta deviation: #{node.var}"
                else
                    node.delta=offsetNew # set the new offset node <-> server without filtering
                    stream_debug "(#{node_ID}) delta to server: #{node.delta}, dev: #{node.var}"
                end
                node.status = status_ID
                node.save
                send_log(node_ID,node.delta,node.var)
            end
            stream_node node.to_json
		when 100..108 # event_ID >= 100: hardware event!
			node = Node.get(node_ID)
			stream_debug "(#{node_ID}) triggered input #{event_ID-100}"
			if node.delta!=nil
				relativeTime = timestamp+node.delta
				event = Event.create(:time => relativeTime, :node_id => node.id)
				# stream it so that a riderId can be connected
                stream_event event.to_json
				node.events << event
				node.save # prevent fuckup when accessing the last event
			end
			node.save
		else
			stream_debug "(#{node_ID}) sent unknown eventID #{event_ID}"
	end
	204 # response without entity body
end
	
# connect event /w rider
post '/api/event/:eventKey' do
	# extract the parameters
	rider_ID = request.body.read.to_i
	event_key = params[:eventKey].to_i
	
	# get the corresponding rider and event
	event = Event.get(event_key)
	rider = Rider.get(rider_ID)
	node_ID = event.node.id || 0
	# create a new rider if it was his first event
	if rider.nil?
		rider = Rider.new(:id => rider_ID)
	end
	
	stream_debug "Connected event #{node_ID}/#{event_key} with rider #{rider_ID}" # TODO, DEBUG: node_ID seems to be 0 all the time
	
	if rider.last.nil? # first of 2 events
		rider.last = event.time
	else # second of 2 events
		diff = (rider.last - event.time).abs
		result = Result.create(:time => diff, :rider_id => rider.id)
		rider.results << result
		# reset
		rider.last = nil
		stream_debug "Run time of #{diff}ms for rider #{rider_ID}"
		stream_result result.to_json
		# log to the sql db a race is running
	 	if $race_id > 0
			send_result($race_id, rider_ID, diff) unless $config['debug'] == '1'
		end
	end
	rider.save
	event.destroy
	204
end


# admin functions

post '/admin/reset' do
	# the redis adapter has no flush function so we need destroy them ‘by hand’
	Rider.all.destroy
	Node.all.destroy
	Event.all.destroy
	Result.all.destroy
    Debug.all.destroy
	erb :admin
end

post '/admin/run/start' do
	$race_id = params['race_id'].to_i
	stream_debug "Race run (#{$race_id}) started."
	204
end

post '/admin/run/stop' do
	stream_debug "Race run (#{$race_id}) stopped."
	$race_id = 0
	204
end

# streaming
################################################################################

# json
get '/stream/event', :provides => 'text/event-stream' do
	stream :keep_open do |out|
		$connections_event << out
		out.callback { $connections_event.delete(out) }
	end
end
get '/stream/node', :provides => 'text/event-stream' do
	stream :keep_open do |out|
		$connections_node << out
		out.callback { $connections_node.delete(out) }
	end
end
get '/stream/result', :provides => 'text/event-stream' do
	stream :keep_open do |out|
		$connections_result << out
		out.callback { $connections_result.delete(out) }
	end
end
get '/stream/debug', :provides => 'text/event-stream' do
	stream :keep_open do |out|
		$connections_debug << out
		out.callback { $connections_debug.delete(out) }
	end
end
# helpers
################################################################################

helpers do
    # json
	def stream_event(data)		
		$connections_event.each { |out| out <<
		"data: #{data}\n\n"}
	end
	def stream_node(data)		
		$connections_node.each { |out| out <<
		"data: #{data}\n\n"}
	end
	def stream_debug(data)
        debug = Debug.create(:time => Time.now.to_i, :text => JSON.generate(data, quirks_mode: true)) # time, text
		$connections_debug.each { |out| out <<
        "data: #{debug.to_json}\n\n" }
        
        Debug.all.drop(32).each { |debug| debug.destroy }
	end
	def stream_result(data)
		$connections_result.each { |out| out <<
		"data: #{data}\n\n"}
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
			con.query "INSERT INTO #{$config['sql_log']['table']} (#{$config['sql_log']['nodeID']},
																   #{$config['sql_log']['nodeDelta']},
																   #{$config['sql_log']['nodeDev']}) VALUES (#{nodeID},
																   #{nodeDelta},
																   #{nodeDev})" 
		rescue Mysql::Error => e
			stream_debug "SQL log reply: #{e.errno}#{e.error}"
		ensure
			con.close if con
		end
	end
end