#!/usr/bin/ruby
 
require "uri"
require "net/http"

def post(node,event,id)
	params = {'NODE' => node,'TSN' => Time.now.to_i ,'EVENT' => event,'ID' => id}
	#x = Net::HTTP.post_form(URI.parse('http://www.satar.l3kn.de/api/event'), params)
	x = Net::HTTP.post_form(URI.parse('http://etemu.com:42337/api/event'), params)
end


for i in 13370..13470 do
post(i,0,0) #init
end