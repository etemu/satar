#!/usr/bin/env ruby -I ../lib -I lib
# coding: utf-8

require 'sinatra' # webserver
require 'erb'		# templates
set :port, 42337
set :server, 'thin'

connections = [] #List all open connections
postcount = 0

get '/' do
	erb :index, :locals => { :count => postcount }
end

get '/stream/' do
	erb :stream
end

get '/test/' do
	erb :index
	#connections.each { |out| out << "data: hello\n\n" }
end

post '/api/time/' do
	puts params
    connections.each { |out| out << "data: #{params['ID']};#{params['TSN']}\n\n"}
    postcount++
    204 # response without entity body
end

get '/stream', :provides => 'text/event-stream' do
	stream :keep_open do |out|
		connections << out
		out.callback { connections.delete(out) }
	end
end
