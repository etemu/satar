<?php
//  etemu.com, Alex Shure CC-BY-SA 3.0
//      ____      ____    _____    ____      ____ CC 
//  ___(_ (_`____/ () \__|_   _|__/ () \____| () )_____   
//    .__)__)   /__/\__\   |_|   /__/\__\   |_|\_\
//  System for Advanced Timekeeping and Amateur Racing.
//
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  !!   Do not change any code in this file.  		  !!
//  !! All configuration is made in config/config.php !!
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
include 'config/config.php';
include 'config/satardb.php';

// store the values of the incoming GET request -> MySQL database

$sql="INSERT INTO $sql_table (T_ID,T,U_ID,U,E,I,N)
VALUES
('$_REQUEST[T_ID]','$_REQUEST[T]','$_REQUEST[U_ID]','$_REQUEST[U]','$_REQUEST[E]','$_REQUEST[I]','$_REQUEST[N]')";

if (!mysql_query($sql,$con))
  {
  die('ERR: DB ' . mysql_error());
  }

echo "ETH: SQL ACK.\n";
mysql_close($con);

// Submit this array to the Ruby API for further processing

$post_data = array(
    'T_ID' => $_REQUEST['T_ID'],
	'T' => $_REQUEST['T'],
	'U_ID' => $_REQUEST['U_ID'],
	'U' => $_REQUEST['U'],
	'EVENT' => $_REQUEST['E'],
	'ID' => $_REQUEST['I'],
    'NODE' => $_REQUEST['N']
);
 
// Send a POST request to the server API 
$result = post_request($ruby_APIhost, $ruby_APIpath, $ruby_APIport, $post_data);
 
if ($result['status'] == 'ok'){
 
	if ($result['content'])
		echo "WRN: !SSR API ERR\n";
		
	if ($result['content']=='')
		echo "ETH: SSR ACK.\n";
	
	// Print headers 
    // echo $result['header']; 
 
    // echo '<hr />';
 
    // print the result of the whole request:
    // echo $result['content'];
	
 
}
else {
    echo 'ERR: SSR API: ' . $result['error']; 
}


function post_request($host, $path, $port, $data, $referer='SatarServer') {
 
    // Convert the data array into URL Parameters like a=b&foo=bar etc.
    $data = http_build_query($data);
 
    // parse the given URL (aShure: deprecated)
    /* $url = parse_url($url);
 
    if ($url['scheme'] != 'http') { 
        die('ERR: Not a http scheme.');
    }
	*/
	
	/*
    // debug host and path:
    //$host = $ruby_APIhost;
	echo 'DEB: ruby_APIhost: ' . $host . '<br>';
    //$path = $ruby_APIpath;
	echo 'DEB: ruby_APIpath: ' . $path . '<br>';
	//$port = $ruby_APIport;
	echo 'DEB: ruby_APIport: ' . $port . '<br>';
	*/
	
    // open a socket connection on port $ruby_APIport - timeout: 8 sec
    $fp = fsockopen($host, $port, $errno, $errstr, 8);
 
    if ($fp){
 
        // send the request headers:
        fputs($fp, "POST $path HTTP/1.1\r\n");
        fputs($fp, "Host: $host\r\n");
 
        if ($referer != '')
            fputs($fp, "Referer: $referer\r\n");
 
        fputs($fp, "Content-type: application/x-www-form-urlencoded\r\n");
        fputs($fp, "Content-length: ". strlen($data) ."\r\n");
        fputs($fp, "Connection: close\r\n\r\n");
        fputs($fp, $data);
 
        $result = ''; 
        while(!feof($fp)) {
            // receive the results of the request
            $result .= fgets($fp, 128);
        }
    }
    else { 
        return array(
            'status' => 'err', 
            'error' => "$errstr ($errno)"
        );
    }
 
    // close the socket connection:
    fclose($fp);
 
    // split the result header from the content
    $result = explode("\r\n\r\n", $result, 2);
 
    $header = isset($result[0]) ? $result[0] : '';
    $content = isset($result[1]) ? $result[1] : '';
 
    // return as structured array:
    return array(
        'status' => 'ok',
        'header' => $header,
        'content' => $content
    );
}

?>
