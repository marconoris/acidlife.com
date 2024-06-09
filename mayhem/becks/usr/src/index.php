<?php
$dirname=getcwd();
$d = dir($dirname);
while($entry=$d->read()) {
    if(is_dir($entry)==1) {
    echo "<a href=\"$entry\" alt=\"$entry\">$entry/</a><BR>\n";
    }
    else {

    if(preg_match("/.visible.php/i",$entry)==1) {
    $entry2=eregi_replace(".visible.php", "",$entry);

    echo "<a href=\"$entry\" alt=\"$entry2\">$entry2</a><BR>\n";
    }
    if(preg_match("/.php/i",$entry)==0) {
    echo "<a href=\"$entry\" alt=\"$entry\">$entry</a><BR>\n";
    }

    }
}
$d->close();
?>

