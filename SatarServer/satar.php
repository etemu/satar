<?php
//  etemu.com, Alex Shure CC-BY-SA 3.0
//      ____      ____    _____    ____      ____ CC 
//  ___(_ (_`____/ () \__|_   _|__/ () \____| () )_____   
//    .__)__)   /__/\__\   |_|   /__/\__\   |_|\_\
//  System for Advanced Timekeeping and Amateur Racing.
//
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  !!   Do not change any code in this file.  !!
//  !! All configuration is made in config.php !!
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
include 'config/config.php';
include 'satardb.php';

$sql="INSERT INTO satar (TSN,EVENT,ID,NODE)
VALUES
('$_REQUEST[TSN]','$_REQUEST[EVENT]','$_REQUEST[ID]','$_REQUEST[NODE]')";

if (!mysql_query($sql,$con))
  {
  die('ERR: ' . mysql_error());
  }
echo "ACK: event added";

mysql_close($con);
?>