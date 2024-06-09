<? include ("menu.txt"); ?>

<!------------------------- CORPO ------------------------->

<div class="ver2">Per sottoscrivere o cancellarsi dalla newsletter: <br>
<form action="http://www.acidlife.com/cgi-bin/mojo/mojo.cgi" method=POST>
        <font face="Verdana, Arial, Helvetica, sans-serif"> <font size="2"> <font face="Georgia, Times New Roman, Times, serif"> 
        <input type="radio" name="flavor" value="subscribe" checked>
        Subscribe 
        <input type="radio" name="flavor" value="unsubscribe">
        Unsubscribe<br>
        <input type="text" name="email" value="email address" size="16">
        <input type="hidden" name="list" value="aidoruproject">
        <input type="submit" value="Submit" name="submit">
        </font> </font> </font> 
      </form></div>

<!------------------------- FINE CORPO ------------------------->

<? include ("piede.txt"); ?>