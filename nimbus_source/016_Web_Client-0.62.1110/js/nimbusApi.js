(function(){	

	var nimbusApi = {	
		
		/*** 
		*Author:Tim
		*Desc:Convert string to JSON.
		*@param {string} str - the string we want to convert.
		*@return {object} - the JSON.
		*/
		str2json:function(str)
		{
			return  eval(t4u.format('({0})',str));
		},
		
		/*** 
		*Author:Tim
		*Desc:To request the Scotti server
		*@param {object} dataJson - the data we want to post.
		*@param {object} funcSucc - the function when we request successfully. 
		*@param {object} funcFial - the function when we request fail. 
		*@param {string} url - If the url is undefined,we will use the default url or we will use this url.
		*@return nothing
		*/
		request: function(dataJson, funcSucc, funcFail, url)
		{			
			var xmlObj=json2xml.convert(dataJson);
			ajaxxml.ajax(xmlObj, funcSucc, funcFail, url);	
		},

		getRemoteItems: function(data)
		{
			var items;
			if(typeof(data.nimbus.items.entry) == 'undefined')
				items = data.nimbus.items;
			else
				items = data.nimbus.items.entry;

			return $.isArray(items) ? items : [items];
		},
		
		/*** 
		*Author:Steven
		*Desc:Get error code. Server returns either errorcode or errcode
		*@param {object} data - return values of server
		*@return error code
		*/
		getErrorCode: function(data)
		{
			if(typeof data.nimbus.errcode != 'undefined')
			{
				return data.nimbus.errcode;
			}
			else if(typeof data.nimbus.errorcode != 'undefined')
			{
				return data.nimbus.errorcode;
			}
			else
			{
				return -1;
			}
		}
	};
	
	window.t4u.nimbusApi = nimbusApi;
})(window);