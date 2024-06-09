<?php
$link = mysql_connect("localhost", "acidlife", "sgtgs.25")
or die ("Could not connect");
print ("Connected successfully");
mysql_select_db ("acidlife")
or die ("Could not select database");
print ("Connected successfullyto db acidlife");

mysql_close($link);
?>
