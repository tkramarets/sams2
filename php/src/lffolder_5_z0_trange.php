<?php
/*
 * SAMS (Squid Account Management System)
 * Author: Dmitry Chemerik chemerik@mail.ru
 * (see the file 'main.php' for license details)
 */

 function lffolder_5_z0_trange()
 {
  global $SAMSConf;
  global $USERConf;

  $DB=new SAMSDB(&$SAMSConf);
  $lang="./lang/lang.$SAMSConf->LANG";
  require($lang);

//  if($SAMSConf->access==2)
//    {
//     print("licenses = insDoc(sams, gLnk(\"D\", \"$lframe_sams_lframe_sams_1\",\"tray.php?show=exe&function=localtraftray\",\"pfile.gif\"))\n");
//    }	 
// if($SAMSConf->access==2 || $SAMSConf->ToUserDataAccess($USERConf->s_user_id, "LC")==1)
    if($USERConf->ToWebInterfaceAccess("C")==1 )
    {
	$item=array("classname"=> "timerange",
		"icon" => "clock.gif",
		"target"=> "basefrm",
		"url"=> "main.php?show=exe&filename=trangetray.php&function=addtrangeform",
		"text"=> "Time Range");
	treeFolder($item);

	$num_rows=$DB->samsdb_query_value("SELECT * FROM timerange");
	while($row=$DB->samsdb_fetch_array())
	{
		$item=array("classname"=> "timerange",
			"target"=> "tray",
			"url"=> "tray.php?show=exe&filename=trangetray.php&function=trangetray&id=$row[s_trange_id]",
			"text"=> "$row[s_name]");
		treeFolderItem($item);
	}
	treeFolderClose();
    }	 

 }
 
 

 ?>