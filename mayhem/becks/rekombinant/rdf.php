<?
$page = file("http://rekombinant.org/backend.php");
$page = implode("", $page);
preg_match_all("/<title\>(.+?)<\/title\>\s+<link\>(.+?)<\/link\>/i", $page, $news, PREG_PATTERN_ORDER);
$i = 0;
while (list(,$match) = each($news[2])) {
	if ($i==0) {
		$i++;
		continue;
	}
	$url = $news[1][$i];
	$SD .= "<li><a target=\"_blank\" href=\"$match\">$url</a>\n";
	$i++;

}
if($SD) {
	$file = fopen("rekombinant.html", "w");
	fputs($file, $SD);
	fclose($file);
}
?>

