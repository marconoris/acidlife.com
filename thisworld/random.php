<?php
$textfile ="http://www.acidlife.com/thisworld/random.txt"; // change to the filename/path of your file

$items = file("$textfile");
$item = rand(0, sizeof($items)-1);
echo $items[$item];
?>
