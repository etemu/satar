#!/usr/bin/env ruby -I ../lib -I lib
# coding: utf-8
#

#########################################################
# SatarServer Ruby by Leon Rische
# version: 0.0.8
# 	+ database stuff
# 	+ system status updating
#########################################################
#  ___      __    __                 
# /\_ \   /'__`\ /\ \                
# \//\ \ /\_\L\ \\ \ \/'\     ___    
#   \ \ \\/_/_\_<_\ \ , <   /' _ `\  
#    \_\ \_/\ \L\ \\ \ \\`\ /\ \/\ \ 
#    /\____\ \____/ \ \_\ \_\ \_\ \_\
#    \/____/\/___/   \/_/\/_/\/_/\/_/
#########################################################

require 'redis'		# model
require 'erb'		# view 
require 'sinatra'	# controller

#########################################################
# setup
#########################################################
	# redis
	#####################################################
	
$redis = Redis.new(:path => "/home/l3kn/.redis/sock")
#$redis = Redis.new(:host => "127.0.0.1", :port => 6379)
$redis.select(2)
$redis.flushdb

	# sinatra
	#####################################################

set :port, 42337
set :server, 'thin'

#########################################################
# variables
#########################################################

$connectionsEvent = [] #List all open $connections
$connectionsSystem = [] #List all open $connections
$connectionsResults = [] #List all open $connections

#########################################################
# routes
#########################################################
	# main views
	#####################################################

get '/' do
	erb :index
end

get '/stream' do
	erb :stream
end

get '/admin' do
	protected!
	erb :admin
end

	# API
	#####################################################

post '/api/event' do
	timestamp = params['TSN'].to_i
	eventId = params['EVENT'].to_i
	nodeId = params['NODE'].to_i
	statusId = params['ID'].to_i

	$connectionsEvent.each { |out| out << "data: #{Time.now.to_i}: N#{nodeId}, E#{eventId}\n\n"}
	case eventId
		when 0
			$connectionsEvent.each { |out| out << "data: #{Time.now.to_i}:(#{nodeId}) booted up\n\n"}
		when 1
			if $redis.sadd('nodes', nodeId) == true
				$redis.hset("node:#{nodeId}",'id',nodeId)
				$connectionsEvent.each { |out| out << "data: #{Time.now.to_i}: Added new node (#{nodeId})\n\n"}
			end
			if $redis.hget("node:#{nodeId}",'status').to_i != statusId
				$redis.hset("node:#{nodeId}",'status',statusId)
				$connectionsEvent.each { |out| out << "data: #{Time.now.to_i}:(#{nodeId}) changed status to #{statusId.to_s(2)}\n\n"}
			end
		else
			$connectionsEvent.each { |out| out << "data: unknown ID\n\n"}
	end
	updateSystemStatus
	204 # response without entity body
end

	# streaming
	#####################################################

get '/api/stream/events', :provides => 'text/event-stream' do
	stream :keep_open do |out|
		$connectionsEvent << out
		out.callback { $connectionsEvent.delete(out) }
	end
end

get '/api/stream/system', :provides => 'text/event-stream' do
	stream :keep_open do |out|
		$connectionsSystem << out
		out.callback { $connectionsSystem.delete(out) }
	end
end


#########################################################
# helpers
#########################################################

helpers do
	def protected!
		unless authorized?
			response['WWW-Authenticate'] = %(Basic realm="Restricted Area")
			throw(:halt, [401, "Not authorized\n"])
		end
	end
	def authorized?
		@auth ||=	Rack::Auth::Basic::Request.new(request.env)
		@auth.provided? && @auth.basic? && @auth.credentials && @auth.credentials == ['admin', 'admin']
	end

	def updateSystemStatus
		base = "<h2>Nodes</h2><ul>"
		for node in allNodes do
			base += "<li>#{node['id']}, status=#{node['status'].to_i.to_s(2)}</li>"
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
end

#########################################################
# protocol
#########################################################
	# params
	#####################################################
	
# TSN - timestamp
# EVENT - event ID
# NODE - node ID
# ID - rider ID

	# packets
	#####################################################

# keepalive/status 
# event = 1 id = 0..255
# 0 0 0 0 0 0 0 0 -> unarmed
# 0 0 0 0 0 0 1 1  -> in 1 & 2 armed

#########################################################
# database
#########################################################
# Sets:
# 
# nodes:all
# nodes:active
# 
# events.nodeID.id
# events:nodeID:id = set
# 
# 
	
