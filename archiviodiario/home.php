<?

require("/usr/local/plesk/apache/vhosts/acidlife.com/httpdocs/news/lib/db_mysql.inc");
require("/usr/local/plesk/apache/vhosts/acidlife.com/httpdocs/news/lib/xtpl.php");
require("/usr/local/plesk/apache/vhosts/acidlife.com/httpdocs/news/lib/common.inc.php");

$xtpl = new XTemplate ("/usr/local/plesk/apache/vhosts/acidlife.com/httpdocs/news/templates/update.xtpl");

if (isset($id)){

  /* SINGLE MODE */

  $test->DB_Sql("select id, titolo, autore, testo, UNIX_TIMESTAMP(tempo) as tempo from news where id = \"$id\" order by tempo desc");
  $test->next_record();
  $xtpl->assign( "TITOLO", $test->f("titolo") );
  $xtpl->assign( "AUTORE", $test->f("autore") );
  $xtpl->assign( "TESTO", $test->f("testo") );
  $xtpl->assign( "DATA", date(d." ".M." ", $test->f("tempo")).(date(Y, $test->f("tempo"))+37) );
  $xtpl->assign( "LINK", $PHP_SELF );
  $xtpl->assign( "BOTTOMLABEL", "index.page");
  $xtpl->parse("main.table");
} else {
  
  /* VIEW MODE */
  
  $test->DB_Sql("select id, titolo, autore, testo, UNIX_TIMESTAMP(tempo) as tempo from news order by tempo desc");
  while ( $test->next_record() ){
    $xtpl->assign( "TITOLO", $test->f("titolo") );
    $xtpl->assign( "AUTORE", $test->f("autore") );
    
    /* taglia il corpo a 300 caratteri (se maggiore di 300) */
    if(strlen($test->f("testo")) > 300){
      $xtpl->assign( "TESTO", substr($test->f("testo"), 0, (strlen($test->f("testo"))-300))."..." ); 
    }else{
      $xtpl->assign( "TESTO", $test->f("testo") );
    };
     
    $xtpl->assign( "DATA",date(d." ".M." ", $test->f("tempo")).(date(Y, $test->f("tempo"))+37) );
    $xtpl->assign( "BOTTOMLABEL", "Read more...");
    $xtpl->assign( "LINK", $PHP_SELF."?id=".$test->f("id") );	
    $xtpl->parse("main.table");
  }
};


$xtpl->parse("main");
$xtpl->out("main");


?>













