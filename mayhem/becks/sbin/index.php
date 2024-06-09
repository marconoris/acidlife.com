<?php
$dirname=getcwd();
$d = dir($dirname);
while($entry=$d->read()) {
    if(is_dir($entry)==1) {
    echo "<a href=\"$entry\" alt=\"$entry\">$entry/</a><BR>\n";
    }
    else {
    if(preg_match("/.php/i",$entry)==0) {
    echo "<a href=\"$entry\" alt=\"$entry\">$entry</a><BR>\n";
    }

    }
}        
$d->close();
?>
