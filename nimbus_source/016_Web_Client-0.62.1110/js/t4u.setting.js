(function(){	

	//var jsonObj={"nimbus":[{"@api":"1.0","@operation":"getDir","path":[{"@start":2,"@count":3,"#cdata":"/storage"}]}]};
	
	var setting = {	
	
		//var dataJson={password:'admin',unique:'think4u'}
		adminLogin: function(funcSucc, funcFail, dataJson)
		{							
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"AdminLogin","password":[{"#text":"'+dataJson.password
													+'"}],"unique":[{"#text":"'+ dataJson.unique +'"}]}]}');
						
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail);
		},
		
		//doesn't call
		//Do not need any data 
		adminLogout: function(funcSucc, funcFail)
		{							
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"AdminLogout"}]}');			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail);
		},
		
		//The following function need to specify the url,because of the token.		
		//var dataJson={password:'admin',hint:'think4u',hintAnswer:'think4u'}
		updateAdminInfo:function(funcSucc, funcFail, dataJson, url)
		{
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"updateAdminInfo","password":[{"#text":"'+dataJson.password
													+'"}],"hint":[{"#text":"'+ dataJson.hint +'"}],"hintAnswer":[{"#text":"'+ dataJson.hintAnswer +'"}]}]}');
													
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url);
		},
		
		//var dataJson={hintAnswer:'think4u'} //dataJson is option.
		getAdminInfo:function(funcSucc, funcFail, dataJson, url)
		{
			var jsonObj;
			
			if(typeof(dataJson.hintAnswer)!='undefined'){			
				jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"getAdminInfo","hintAnswer":[{"#text":"'+dataJson.hintAnswer+'"}]}]}');
			}else{
				var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"getAdminInfo"}]}');	
			}
			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url);
		},
		
		//var dataJson={name:'Galexy S3'} //dataJson is option.
		changeDeviceName:function(funcSucc, funcFail, dataJson, url)
		{
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"changeDeviceName","name":[{"#text":"'+dataJson.name+'"}]}]}');
			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url);
		},
		
		//Do not need any data 
		//Do not need url
		getDeviceName: function(funcSucc, funcFail, url)
		{							
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"getDeviceName"}]}');			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url);
		},
		
		//Do not need any data 
		getConnectedDevices: function(funcSucc, funcFail, url)
		{							
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"getConnectedDevices"}]}');			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url);
		},
		
		//doesn't call
		//Do not need any data  
		disconnectNimbus: function(funcSucc, funcFail, url)
		{							
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"disconnectNimbus"}]}');			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url);
		},
		
		//The following function need to specify the url,because of the token.
		//Wi-Fi AP settings
		
		//var dataJson={name:'Nimbus AP',security:'WAP',password:'nimbus001'}
		setSSID:function(funcSucc, funcFail, dataJson, url)
		{
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"setSSID","name":[{"#text":"'+dataJson.name
													+'"}],"security":[{"#text":"'+ dataJson.security +'"}],"password":[{"#text":"'+ dataJson.password +'"}]}]}');
													
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url);
		},
		
		//Do not need any data 
		getSSID: function(funcSucc, funcFail, url)
		{							
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"getSSID"}]}');			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url);
		},
		
		//Do not need any data 
		getAPList: function(funcSucc, funcFail)
		{							
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"getAPList"}]}');			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail);
		},
		
		//Do not need any data 
		getConnectedAPInfo: function(funcSucc, funcFail)
		{							
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"getConnectedAPInfo"}]}');			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail);
		},
		
		//var dataJson={name:'Nimbus AP',security:'WAP',password:'nimbus001'}
		connectAP:function(funcSucc, funcFail, dataJson)
		{
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"connectAP","name":[{"#cdata":"'+dataJson.name
													+'"}],"security":[{"#text":"'+ dataJson.security +'"}],"password":[{"#text":"'+ dataJson.password +'"}]}]}');
													
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail);
		},
		
		//var dataJson={name:'Nimbus AP'}
		reConnectAP:function(funcSucc, funcFail, dataJson)
		{
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"reConnectAP","name":[{"#cdata":"'+dataJson.name+'"}]}]}');													
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail);
		},
		
		//var dataJson={internetAP:true,MSCMode:true,DNLA:true,FTP:true,Bonjour:true}
		setUseServices:function(funcSucc, funcFail, dataJson, url)
		{
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"setUseServices","internetAP":[{"#text":"'+dataJson.internetAP
													+'"}],"MSCMode":[{"#text":"'+ dataJson.MSCMode 
													+'"}],"DNLA":[{"#text":"'+ dataJson.DNLA+'"}],"FTP":[{"#text":"'
													+ dataJson.FTP+'"}],"Bonjour":[{"#text":"'+ dataJson.Bonjour+'"}]}]}');											
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url);
		},
		
		//Do not need any data 
		getUseServices:function(funcSucc, funcFail, url)
		{
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"getUseServices"}]}');			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url);
		},
		
		//Do not need any data 		
		//Do not need the token
		getBatteryLevel:function(funcSucc, funcFail)
		{
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"getBatteryLevel"}]}');			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail);
		},
		
		//Reset Nimbus
		//Do not need the token
		
		//doesn't call
		//var dataJson={name:'My Nimbus'}
		setNimbusName:function(funcSucc, funcFail, dataJson)
		{
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"setNimbusName","name":[{"#text":"'+dataJson.name+'"}]}]}');										
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail);
		},
		
		//Do not need any data 	
		getNimbusInfo:function(funcSucc, funcFail)
		{
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"getNimbusInfo"}]}');			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail);
		},
		
		//var dataJson={ssidName:true,ssidPassword:true,adminPassword:true,deleteAllAPlist:true}
		resetNimbus:function(funcSucc, funcFail, dataJson,url)
		{
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"resetNimbus","ssidName":[{"#text":"'+dataJson.ssidName
													+'"}],"ssidPassword":[{"#text":"'+ dataJson.ssidPassword 
													+'"}],"adminPassword":[{"#text":"'+ dataJson.adminPassword+'"}],"deleteAllAPlist":[{"#text":"'
													+ dataJson.deleteAllAPlist+'"}]}]}');									
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url);
		},
		
		//var dataJson={internal:true,sd:true}
		formatNimbus:function(funcSucc, funcFail, dataJson, url)
		{
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"formatNimbus","internal":[{"#text":"'+dataJson.internal
													+'"}],"sd":[{"#text":"'+ dataJson.sd +'"}]}]}');									
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url);
		},
		
		//Do not need any data 	
		getFirmwareInfo:function(funcSucc, funcFail, url)
		{
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"getFirmwareInfo"}]}');			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url);
		},
		
		//Do not need any data 	
		upgradeFirmware:function(funcSucc, funcFail, url)
		{
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"upgradeFirmware"}]}');			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url);
		},
		
		//doesn't call
		//Do not need any data 	
		//Error:post error
		waitSDEvent:function(funcSucc, funcFail)
		{
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"waitSDEvent"}]}');			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail);
		}
		
	};
	
	window.t4u.nimbusApi.setting = setting;
})(window);