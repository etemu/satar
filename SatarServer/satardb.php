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
$con = mysql_connect($sql_host, $sql_user, $sql_pass);
if (!$con)
  {
  die('ERR: Database: ' . mysql_error());
  }
mysql_select_db($sql_db, $con);
?>