
<p>&nbsp;</p>
<p>Links</p>
<p>&nbsp;</p>
<p><?

 $f= fopen("txt/links.txt", "r")
   or die("unable to open file");

 print("<ul type = circle>");

 while(!feof($f)) {

   $line= substr(fgetline($f), 8);

   if($line[0]==' ')
    print("<li><p>$line</p>");
   else
    print("<p><a href=\"$line\">$line</a></p>");

   }

 fclose($f);

 print("<ul>");

?></p>
