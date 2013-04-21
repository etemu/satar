<?php
//  etemu.com, Alex Shure CC-BY-SA-NC 3.0
//      ____      ____    _____    ____      ____ CC 
//  ___(_ (_`____/ () \__|_   _|__/ () \____| () )_____   
//    .__)__)   /__/\__\   |_|   /__/\__\   |_|\_\
//  System for Advanced Timekeeping and Amateur Racing.
//
// This is the configuration for the MySQL database connection.  
// Change the values below to your MySQL server data.
// The MySQL table name is hardcoded and will be "satar".
//
$sql_host		= 'localhost'; 		// the database server, e.g. localhost
$sql_user 		= 'USER'; 			// username who with read+write access
$sql_pass 		= 'PASSWORD'; 		// the password
$sql_db 		= 'satar'; 			// the database which is used
$sql_table		= 'satar-raw'		// the database table which is used (should exist)

$ruby_APIhost = 'localhost'; 		// Host adress of the Ruby Server.
$ruby_APIport = '42337'; 			// Port of the Ruby Server.
$ruby_APIpath = '/api/event' 		// Path to the SatarServerRuby's API

?>