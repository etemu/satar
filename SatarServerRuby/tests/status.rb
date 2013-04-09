#!/usr/bin/ruby
 
require "uri"
require "net/http"

def post(node,event,id)
	params = {'NODE' => node,'TSN' => Time.now.to_i ,'EVENT' => event,'ID' => id}
	x = Net::HTTP.post_form(URI.parse('http://www.satar.l3kn.de/api/event'), params)
end

delay = 1

for i in 10..15 do
node = i
post(node,0,0) #init
sleep(delay)
post(node,1,1) #init
sleep(delay)
post(node,1,11) #init
sleep(delay)
post(node,1,111) #init
sleep(delay)
end