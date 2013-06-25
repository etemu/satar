#!/usr/bin/ruby
 
require "uri"
require "net/http"

def post1(node,event,id)
  params = {'NODE' => node,'TSN' => (Time.now.to_f-10000) ,'EVENT' => event,'ID' => id}
	x = Net::HTTP.post_form(URI.parse('http://localhost:42337/api/event'), params)
end

def post2(node,event,id)
	params = {'NODE' => node,'TSN' => (Time.now.to_f+100) ,'EVENT' => event,'ID' => id}
	x = Net::HTTP.post_form(URI.parse('http://localhost:42337/api/event'), params)
end

post1(1,1,111) #init
post2(2,1,111) #init
post1(1,101,111) #init
post2(2,101,111) #init
