(function(){
	
	/*** 
		Author:Tim
		Desc:remove the useless character(s) or character '&'.
		@param {object} xmlObj - its type is xml.
		@return {object} -  its type is xml and it does not contain useless character(s) and character '&'.
	*/
	function preProcessXml(xmlObj)
	{
		var str=convert_xml_to_string(xmlObj);
		str=str.replace(/<!\[CDATA\[</g,'');
		str=str.replace(/>\]\]>/g,'');
		str=str.replace(/<!\[CDATA\[/g,'');
		str=str.replace(/\]\]>/g,'');
	
		str=str.replace(/\&/g,'/CandC/');
		
		if(str.indexOf('<nimbus errcode="0">Success</nimbus>')!=-1)
			str=str.replace('Success','');   //process success msg,if it does not have any tag.
		
		return convert_string_to_xml(str);
	}

	/*** 
		Author:Tim
		Desc: The external library will convert the xml to JSON.
		@param {object} xmlObj - its type is xml.
		@return {object} -  its type is JSON.
	*/
	function xmlToJson(xmlObj) {
	  
	  // Create the return object
	  var obj = {};

	  if (xmlObj.nodeType == 1) { // element
		// do attributes
		if (xmlObj.attributes.length > 0) {   
		  for (var j = 0; j < xmlObj.attributes.length; j++) {
			var attribute = xmlObj.attributes.item(j);		
			obj[attribute.nodeName] = attribute.nodeValue;
		  }
		}
	  } else if (xmlObj.nodeType == 3) { // text 
		obj = xmlObj.nodeValue;
	  }

	  // do children
	  if (xmlObj.hasChildNodes()) {
		for(var i = 0; i < xmlObj.childNodes.length; i++) {
		  var item = xmlObj.childNodes.item(i);
		  var nodeName = item.nodeName;
		  if (typeof(obj[nodeName]) == "undefined") {        
			obj[nodeName] = xmlToJson(item);		
		  } else {
			if (typeof(obj[nodeName].length) == "undefined") {
			  var old = obj[nodeName];
			  obj[nodeName] = [];
			  obj[nodeName].push(old);
			}
			obj[nodeName].push(xmlToJson(item));
		  }
		}
	  }
	  return obj;
	} 
 
	/*** 
		Author:Tim
		Desc: The function will simplify the response, but there has some problem about the some '[#text]' can not remove all.
		@param {object} jsonObj - the data type is JSON.
		@return {object} -  the simplier JSON.
	*/
	function simplify(jsonObj)
	{
		var str=JSON.stringify(jsonObj);

		str=str.replace(/\/CandC\//g,'&');  
		
		var ptrn=/{\"#text\":\"[^\"]*\"}/g;
		var match;
		while ( ( match = ptrn.exec(str) ) != null )
		{
			var want=match[0].replace(/"#text"/g,'').split(':');
			if(want.length==2){
				str=str.replace(match[0], match[0].replace(/"#text"/g,'').split(':')[1].replace(/}$/g,''));
			}else{
				var wantStr='';
				for(var i=1;i<want.length;i++){
					if(i>1)
						wantStr+=':';
					wantStr+=want[i];
				}
				wantStr=wantStr.replace(/}$/g,'')
				str=str.replace(match[0],wantStr);
			}
			
		}
		
		//Convert string to boolean
		str=str.replace(/"true"/g,'true');
		str=str.replace(/"false"/g,'false');
		
		//Convert string to number
		var ptrn=/"[\d]+"/g;
		var match;
		while ( ( match = ptrn.exec(str) ) != null )
		{
			str=str.replace(match[0], match[0].replace(/"/g,'') );
		}
		
		return eval('(' + str+ ')');
	}
	
	/*** 
		Author:Tim
		Desc: convert string to xml.
		@param {string} xmlStr - the string.
		@return {object} -  the type is xml.
	*/
	function convert_string_to_xml(xmlStr)
	{
		if(xmlStr.indexOf("<?")==0) {
			xmlStr = xmlStr.substr( xmlStr.indexOf("?>") + 2 );
		}
		//trim strange code
		xmlStr = xmlStr.replace(new RegExp(String.fromCharCode(65535), 'g'), '');
		xmlStr = xmlStr.replace(new RegExp(String.fromCharCode(3), 'g'), '');
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
	
	/*** 
		Author:Tim
		Desc: convert xml to string.  
		@param {object} xmlStr - the type is xml.
		@return {string} -  the string.
	*/
	function convert_xml_to_string(xmlObj)
	{
		if (window.ActiveXObject) { // for IE
			if(typeof xmlObj.xml === 'undefined')
				return (new XMLSerializer()).serializeToString(xmlObj);
			else
			return xmlObj.xml;
		} else {
			return (new XMLSerializer()).serializeToString(xmlObj);
		}
	}
	
	
	var xml2json = {	
	
		/*** 
			Author:Tim
			Desc: convert xml to string.  
			@param {object} obj - the type is xml.
			@return {object} -  the type is JSON and it is simply.
		*/
		convert: function(obj)
		{		
			if(typeof(obj)=='string')
				obj=convert_string_to_xml(obj);
			
			obj=preProcessXml(obj);
			return simplify(simplify(xmlToJson(obj)));
		}
	};
	
	window.xml2json = xml2json;
})(window);