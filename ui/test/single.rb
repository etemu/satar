require 'uri'
require 'net/http'

#$url = 'http://jack.l3kn.de'
$url = 'http://localhost:42337'

def post(node,event,id)
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


post(1,0,111)
post(1,1,111)
10.times { post(2,101,111) }
