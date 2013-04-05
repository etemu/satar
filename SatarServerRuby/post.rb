#!/usr/bin/ruby
 
require "uri"
require "net/http"
100.times do
	params = {'msg' => Time.now}
	x = Net::HTTP.post_form(URI.parse('http://www.satar.l3kn.de/'), params)
	puts x.body
	sleep(10)
end