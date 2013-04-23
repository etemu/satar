#!/usr/bin/ruby
 
require "uri"
require "net/http"

#$url = 'http://www.area42.l3kn.de'
$url = 'http://localhost:42337'

def post1(node,event,id)
	ts = (Time.now.to_f*1000).floor
	puts ts
	params = {'NODE' => node,'TSN' => (ts-1000.0) ,'EVENT' => event,'ID' => id}
	x = Net::HTTP.post_form(URI.parse("#{$url}/api/event"), params)
end

def post2(node,event,id)
	ts = (Time.now.to_f*1000).floor
	puts ts
	params = {'NODE' => node,'TSN' => (ts+1000.0) ,'EVENT' => event,'ID' => id}
	x = Net::HTTP.post_form(URI.parse("#{$url}/api/event"), params)
end


#post1(1,101,111) #init
post2(2,0,111)
post2(2,1,111)
post2(2,101,111)
post2(2,101,111)
#for j in 1..20 do
#for i in 1..5 do
#	post1(i,101,0) #init
#	sleep(0.01)
#end
#end
