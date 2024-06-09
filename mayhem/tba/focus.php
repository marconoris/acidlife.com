<html>
<head>
<title>xe headQ</title>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
</head>
<body text="#333333" link="#6699CC" vlink="#6699CC" alink="#6699CC" bgcolor="#FFFFFF">
<div align="center"> 
  <p><font face="Courier New, Courier, mono"><b><a href="focus.php?page=home"><img src="x_eyes.gif" width="248" height="70" border="0"></a></b></font></p>
  <p><font face="Courier New, Courier, mono"><b><a href="focus.php?page=home"><font size="-1">home</font></a><font size="-1"> 
    <font size="+2">|</font> <a href="focus.php?page=links">links</a> </font><font face="Courier New, Courier, mono"><b><font size="-1"><font size="+2"> 
    |</font></font></b></font><font size="-1"> <a href="focus.php?page=projects"> 
    projects</a> </font><font face="Courier New, Courier, mono"><b><font size="-1"><font size="+2"> 
    | </font></font></b></font><font size="-1"><a href="mailto:xenion@acidlife.com">mailme 
    </a></font><font face="Courier New, Courier, mono"><b><a href="focus.php?page=home"></a></b></font></b></font></p>
  <p><font face="Courier New, Courier, mono"><b><font face="Courier New, Courier, mono"><b><a href="focus.php?page=home"></a></b></font></b></font></p>
  <table width="800" border="0" cellpadding="0" cellspacing="0">
    <tr> 
      <td> 
        <p align="left"><?


function fgetline($f) {

  $line = fgets($f, 4096);

  if(!$line)
   return($line);

  return(trim($line));
}

if(empty($page))
  $page= "home";

if(file_exists($page.".php"))
  require($page.".php");
else
  print("page not found");  

?></p>
      </td>
    </tr>
  </table>
  
  <p>&nbsp;</p>
  </div>
</body>
</html>
