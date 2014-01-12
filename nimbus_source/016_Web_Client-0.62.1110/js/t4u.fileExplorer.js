(function(){	

	//var jsonObj={"nimbus":[{"@api":"1.0","@operation":"getDir","path":[{"@start":2,"@count":3,"#cdata":"/storage"}]}]}; //original json format
	
	var fileExplorer = {	
		
		//var dataJson={start:3,count:2,path:'/storage'} //start and count is option
		getDir: function(funcSucc, funcFail, dataJson)
		{
			var pathStr=JSON.stringify(dataJson);
			pathStr=pathStr.replace('start','@start');
			pathStr=pathStr.replace('count','@count');
			pathStr=pathStr.replace('path','#cdata');				
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"getDir","path":['+pathStr+']}]}');
			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url == "" ? undefined : url);
		},
		
		//doesn't call
		//var dataJson={path:'/storage'}
		getInfo: function(funcSucc, funcFail, dataJson)
		{
			var pathStr=JSON.stringify(dataJson);			
			pathStr=pathStr.replace('path','#cdata');				
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"getInfo","path":['+pathStr+']}]}');
			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url == "" ? undefined : url);
		},
		
		//doesn't call
		//var dataJson={path:'/storage/error_log.txt'}
		//Error:GET http://192.168.18.136:10101/NimbusAPI/ 404 (Not Found)
		getThumbnail: function(funcSucc, funcFail, dataJson)
		{
			var pathStr=JSON.stringify(dataJson);			
			pathStr=pathStr.replace('path','#cdata');				
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"getThumbnail","path":['+pathStr+']}]}');
			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url == "" ? undefined : url);
		},
		
		//var dataJson={'forced':false,'path':'/music/a.mp3'};
		deletePath: function(funcSucc, funcFail, dataJson)
		{
			var forced;
			(typeof(dataJson.forced)=="undefined")? forced=false:forced=dataJson.forced;
			
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"delete","entry":[{"@forced":"'+forced+'","path":[{"#cdata":"'+dataJson.path+'"}]}]}]}');
			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url == "" ? undefined : url);
		},
		
		//var dataJson={'src':'/storage','dest':'/funny'};
		copyRenameMove: function(operation, funcSucc, funcFail, dataJson)
		{
			
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"'+operation+'","entry":[{"@forced":"' + (dataJson.force == true) + '", "src":[{"#cdata":"'+dataJson.src+'"}],"dest":[{"#cdata":"'+dataJson.dest+'"}]}]}]}');
			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url == "" ? undefined : url);
		},
		
		copy: function(funcSucc, funcFail, dataJson)
		{
			return t4u.nimbusApi.fileExplorer.copyRenameMove('copy', funcSucc, funcFail, dataJson);
		},		
		
		//doesn't call
		rename: function(funcSucc, funcFail, dataJson)
		{
			return t4u.nimbusApi.fileExplorer.copyRenameMove('rename', funcSucc, funcFail, dataJson);
		},
		
		//doesn't called
		move: function(funcSucc, funcFail, dataJson)
		{
			return t4u.nimbusApi.fileExplorer.copyRenameMove('move', funcSucc, funcFail, dataJson);
		},
		
		//doesn't call
		//var dataJson={'path':'/storage/happy'}
		mkdir: function(funcSucc, funcFail, dataJson)
		{
			var pathStr=JSON.stringify(dataJson);			
			pathStr=pathStr.replace('path','#cdata');				
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"mkdir","path":['+pathStr+']}]}');
			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url == "" ? undefined : url);
		},
		
		//var dataJson={start:3,count:2,path:'/storage'} //start and count is option.
		find: function(funcSucc, funcFail, dataJson, keywordStr)
		{
			var pathStr=JSON.stringify(dataJson);
			pathStr=pathStr.replace('start','@start');
			pathStr=pathStr.replace('count','@count');
			pathStr=pathStr.replace('path','#cdata');				
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"find","path":['+pathStr+'],"keyword":{"#cdata":"'+keywordStr+'"}}]}');
			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url == "" ? undefined : url);
		},
		
		//Media Start
		//var dataJson={'mediaType':[{'start':2,'count':3,'type':'music'}],'sort':[{'order':'asc','field':'title'}]};  //sort is option.
		//var filter={filter:[{'field':'Artist','value':'Beatles'},{'field':'cc','value':'dd'}]}; //it is option.
		getList: function(operation, funcSucc, funcFail, dataJson, filter)
		{
			var typeStr=JSON.stringify(dataJson);
			typeStr=typeStr.replace('start','@start');
			typeStr=typeStr.replace('count','@count');
			typeStr=typeStr.replace('type','#text');
			typeStr=typeStr.replace('order','@order');	
			typeStr=typeStr.replace('field','#text');
			typeStr=typeStr.replace('dir','#text');   //category
			typeStr=typeStr.substring(1,typeStr.length-1);
			
			
			
			if(typeof(filter)!="undefined")
			{
				var filterStr=JSON.stringify(filter);
				var ptrnField=/field/g;
				var ptrnValue=/value/g;
				filterStr=filterStr.replace(ptrnField,'@category');
				filterStr=filterStr.replace(ptrnValue,'#cdata');
				
			}
			
			
			
			var jsonObj;
			if(typeof(filter)!="undefined")
				jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"'+operation+'",'+typeStr+',"items":['+filterStr+']}]}');
			else
				jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"'+operation+'",'+typeStr+'}]}');
			
				
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail, url == "" ? undefined : url);
		},
		
		//var dataJson={'mediaType':[{'start':2,'count':3,'type':'music'}],'sort':[{'order':'asc','field':'title'}]};  //sort is option.
		//var filter={filter:[{'field':'Artist','value':'Beatles'},{'field':'cc','value':'dd'}]}; //it is option.
		getMediaList: function(funcSucc, funcFail, dataJson, filter)
		{
			return t4u.nimbusApi.fileExplorer.getList('getMediaList', funcSucc, funcFail, dataJson, filter);
		},
		
		//var dataJson={'category':[{'dir':'Genre'}],'mediaType':[{'start':2,'count':3,'type':'music'}],'sort':[{'order':'asc','field':'title'}]};  //sort is option.
		//var filter={filter:[{'field':'Artist','value':'Beatles'},{'field':'cc','value':'dd'}]}; //it is option.
		//Problem: We will need to know the field name.
		getCategoryList: function(funcSucc, funcFail, dataJson, filter)
		{
			return t4u.nimbusApi.fileExplorer.getList('getCategoryList', funcSucc, funcFail, dataJson, filter);
		},
		
		getCategoryThumbnailList: function(funcSucc, funcFail, dataJson, filter)
		{
			return t4u.nimbusApi.fileExplorer.getList('getCategoryThumbnailList', funcSucc, funcFail, dataJson, filter);
		},
		
		searchMediaList: function(funcSucc, funcFail, dataJson, filter)
		{
			return t4u.nimbusApi.fileExplorer.getList('searchMediaList', funcSucc, funcFail, dataJson, filter);
		},
		
		//doesn't call
		//Do not need any data
		mediaScan: function(funcSucc, funcFail)
		{							
			var jsonObj=t4u.nimbusApi.str2json('{"nimbus":[{"@api":"1.0","@operation":"mediaScan"}]}');			
			return t4u.nimbusApi.request(jsonObj, funcSucc, funcFail);
		}
		
	};
	
	window.t4u.nimbusApi.fileExplorer = fileExplorer;
})(window);