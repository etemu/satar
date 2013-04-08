#!/usr/bin/env ruby -I ../lib -I lib
# coding: utf-8
#

#########################################################
# SatarServer Ruby by Leon Rische
# version: 0.0.7
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
	
# $redis = Redis.new(:path => "/home/l3kn/.redis/sock")
redis = Redis.new(:host => "127.0.0.1", :port => 6379)
redis.select(2)

	# sinatra
	#####################################################

set :port, 42337
set :server, 'thin'

#########################################################
# variables
#########################################################

connectionsEvent = [] #List all open connections
connectionsSystem = [] #List all open connections
connectionsResults = [] #List all open connections


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
	#connections.each { |out| out << "data: hello\n\n" }
end

	# API
	#####################################################

post '/api/event' do
	eventId = params['EVENT'].to_i
	nodeId = params['NODE'].to_i
	statusId = params['ID'].to_i
	case eventId
		when 0
			$redis.sadd('nodes:all', nodeId)
			case statusId
				when 0

				else

			end 	
	end

	connectionsEvent.each { |out| out << "data: #{params['ID']};#{params['TSN']}\n\n"}
	204 # response without entity body
end

	# streaming
	#####################################################

get '/api/stream/events', :provides => 'text/event-stream' do
	stream :keep_open do |out|
		connectionsEvent << out
		out.callback { connectionsEvent.delete(out) }
	end
end

get '/api/stream/system', :provides => 'text/event-stream' do
	stream :keep_open do |out|
		connectionsSystem << out
		out.callback { connectionsSystem.delete(out) }
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
	
