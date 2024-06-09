
<p>&nbsp;</p>
<p>Projects, articoli, hacks. Alcune cose sono presenti anche sul cvs 
  di antifork: <a href="http://cvs.antifork.org">http://cvs.antifork.org</a> </p>
<p>
<p>&nbsp;</p>
<?php

$path= "projects/";
$pathname= "txt/projects.txt";

function display_size($file){
 $file_size = filesize($file);
 if($file_size >= 1073741824)
   	{
         $file_size = round($file_size / 1073741824 * 100) / 100 . "g";
 	}
 elseif($file_size >= 1048576)
 	{
         $file_size = round($file_size / 1048576 * 100) / 100 . "m";
	}
 elseif($file_size >= 1024)
	{
         $file_size = round($file_size / 1024 * 100) / 100 . "k";
	}
 else{
         $file_size = $file_size . "b";
	}
 return $file_size;
}


function add($f, $path) {
  if(strcmp(fgetline($f), "<new>")!=0)
   return(0);

  $name= substr(fgetline($f), 16);
  $what= substr(fgetline($f), 16);

  print ("<li> <a href=\"$path$name\">$name</a> [".display_size($path.$name)."]");
  print ("<p>$what</p>");

  return(1);
  }


$f = fopen($pathname, "r")
        or die("unable to open file");

 print("<ul type= circle>");

 while(add($f, $path))
  fgetline($f);

 fclose($f);

 print("</ul>");
?> </p>
