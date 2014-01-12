(function(){
	
	/*** 
		Author: Tim
		Desc: fail_callback
	*/
	/*
	function fail_callback( data, textStatus, jqXHR)
	{
		//t4u.showMessage(multiLang.search('Wireless Media Drive server stop to service.'));
	}
	*/
	
	var ajaxxml = {	
		/*** 
			Author:Tim
			Desc: To post the JSON data and get the xml response,and we will change the xml response to JSON
			@param {object} dataObj - the data we want to post.
			@param {object} funcSucc - the function when we request successfully. 
			@param {object} funcFail - the function when we request fail. 
			@param {string} url - If the url is undefined,we will use the default url or we will use this url.
			@return nothing
		*/
		ajax: function(dataObj, funcSucc, funcFail, url)
		{		
			
			if(typeof(dataObj)=='string')
				dataObj=ajaxxml.str2xml(dataObj);
			
			var success_callback=function(data, textStatus, jqXHR)
			{
				if(typeof data == 'undefined')
					return;
				var dataJson=xml2json.convert(data);
				var str=JSON.stringify(dataJson);
				str=str.replace('"xml":{},','');
				dataJson=$.secureEvalJSON(str);
				if(t4u.nimbusApi.getErrorCode(dataJson) != 0)
				{
					if(dataJson.nimbus.errmsg != 'Fail')
					{
						if(dataJson.nimbus.errcode == 116)
						{
							var obj = {title:multiLang.search('USBOnTitle'), message:multiLang.search('USBOnMessage')};
							t4u.showMessage(obj);
						}
						else						
						{
							if(typeof dataJson.nimbus.errmsg === 'string')
						t4u.showMessage(dataJson.nimbus.errmsg);
							else
								t4u.showMessage(errCode[dataJson.nimbus.errcode]);
						}
				}
				}
				funcSucc.call(this,dataJson);
			};

			var fail_callback=function(data, textStatus, jqXHR)
			{
				funcFail.call(this, textStatus);
			};
			
			//var datatype = 'xml';
			var datatype = 'text';
			var contentType = 'text/xml';
			
			if(typeof(url)=='string')						
				t4u.request(url,dataObj,success_callback, fail_callback, datatype, contentType);	
			else
				t4u.request(xmlUrl,dataObj,success_callback, fail_callback, datatype, contentType);	
	
		},
		
		/*** 
			Author:Tim
			Desc: Convert the string to xml.
			@param {string} xmlStr - the string of xml.
			@return {object} - xml which is converted by xmlStr.
		*/
		str2xml:function(xmlStr)
		{
			if (window.ActiveXObject) {
				try
				{
				xmlDoc=new ActiveXObject("Microsoft.XMLDOM");
				xmlDoc.async="false";
				xmlDoc.loadXML(xmlStr);
				return xmlDoc;
				}
				catch(e)
				{
					parser=new DOMParser();
					xmlDoc=parser.parseFromString(xmlStr,"text/xml");
					return xmlDoc;
				}
			} else {
				parser=new DOMParser();
				xmlDoc=parser.parseFromString(xmlStr,"text/xml");
				return xmlDoc;
			}
		}
	};
	
	window.ajaxxml = ajaxxml;
})(window);