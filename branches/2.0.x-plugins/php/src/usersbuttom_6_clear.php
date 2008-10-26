<?php
/*  
 * SAMS (Squid Account Management System)
 * Author: Dmitry Chemerik chemerik@mail.ru
 * (see the file 'main.php' for license details)
 */

function ClearUsersTrafficCounter()
{
  global $SAMSConf;
  $DB=new SAMSDB("$SAMSConf->DB_ENGINE", $SAMSConf->ODBC, $SAMSConf->DB_SERVER, $SAMSConf->DB_USER, $SAMSConf->DB_PASSWORD, $SAMSConf->SAMSDB, $SAMSConf->PDO);
  $lang="./lang/lang.$SAMSConf->LANG";
  require($lang);

  $SAMSConf->access=UserAccess();
  if($SAMSConf->access==2 || $SAMSConf->ToUserDataAccess($USERConf->s_user_id, "C")==1)
    {
      $num_rows=$DB->samsdb_query("UPDATE squiduser SET s_size='0', s_hit='0' ");
//      UpdateLog("$SAMSConf->adminname","$usersbuttom_6_clear_ClearUsersTrafficCounter","01");
	print("<SCRIPT>\n");
	print("        parent.basefrm.location.href=\"main.php?show=exe&filename=userstray.php&function=AllUsersForm&type=all\";\n");
	print("</SCRIPT> \n");
    }  
}



function usersbuttom_6_clear()
{
  global $SAMSConf;
  $lang="./lang/lang.$SAMSConf->LANG";
  require($lang);

  if($SAMSConf->access==2 || $SAMSConf->ToUserDataAccess($USERConf->s_user_id, "C")==1)
    {
       print("<SCRIPT language=JAVASCRIPT>\n");
       print("function ReloadBaseFrame()\n");
       print("{\n");
       print("   window.location.reload();\n");
       print("}\n");
       print("function ClearCounter(username,userid)\n");
       print("{\n");
       print("  value=window.confirm(\"$usersbuttom_6_clear_usersbuttom_6_clear_1? \" );\n");
       print("  if(value==true) \n");
       print("     {\n");
       print("        parent.basefrm.location.href=\"main.php?show=exe&function=clearuserstrafficcounter&filename=usersbuttom_6_clear.php\";\n");
	   print("        window.setInterval(\"ReloadBaseFrame()\",500)\n");
       print("     }\n");
       print("}\n");
       print("</SCRIPT> \n");

       print("<TD VALIGN=\"TOP\" WIDTH=\"50\" HEIGHT=\"50\">\n");
       print("<IMAGE id=Trash name=\"Clear\" src=\"$SAMSConf->ICONSET/erase_32.jpg\" \n ");
       print("TITLE=\"$usersbuttom_6_clear_usersbuttom_6_clear_2\"  border=0 ");
       print("onclick=ClearCounter(\"nick\",\"id\") \n");
       print("onmouseover=\"this.src='$SAMSConf->ICONSET/erase_48.jpg'\" \n");
       print("onmouseout= \"this.src='$SAMSConf->ICONSET/erase_32.jpg'\" >\n");
    }

}




?>