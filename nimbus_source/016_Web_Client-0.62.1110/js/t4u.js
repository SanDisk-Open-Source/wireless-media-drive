(function(){
var core;
var ajaxQuery = $.manageAjax.create('ajaxQueue', {
	queue: true,
	maxRequests: 2,
	preventDoubleRequests: false,
	queueDuplicateRequests: true
});
if (core && (typeof core != "object" || core.NAME))
    throw new Error("The base namespace 'core' already exists");

// Create our namespace
core = {};

// This is some metainformation about this namespace
core.NAME = "t4u";    // The name of this namespace

// This is the list of public symbols that we export from this namespace.
// These are of interest to programers who use modules.
core.EXPORT = ["getInstance", "require"];

// These are other symbols we are willing to export. They are ones normally
// used only by module authors and are not typically imported.
core.EXPORT_OK = ["namespace", "isDefined",
                    "modules", "globalNamespace"];

// This deposit 3rd part objects
core.REGISTER = {};

// Now start adding symbols to the namespace
core.globalNamespace = this;  // So we can always refer to the global scope
core.modules = { "t4u": core };  // Module name->namespace map.

/***
 * This function creates and returns a namespace object for the
 * specified name and does useful error checking to ensure that the
 * name does not conflict with any previously loaded module. It
 * throws an error if the namespace already exists or if any of the
 * property components of the namespace exist and are not objects.
 *
 * Sets a NAME property of the new namespace to its name.
 * If the version argument is specified, set the VERSION property
 * of the namespace.
 *
 * A mapping for the new namespace is added to the Module.modules object
 */
core.namespace = function(name, init) {
    // Check name for validity.  It must exist, and must not begin or
    // end with a period or contain two periods in a row.
    if (!name) throw new Error("core.createNamespace( ): name required");
    if (name.charAt(0) == '.' ||
        name.charAt(name.length-1) == '.' ||
        name.indexOf("..") != -1)
        throw new Error("core.createNamespace( ): illegal name: " + name);

    // Break the name at periods and create the object hierarchy we need
    var parts = name.split('.');
    
    // For each namespace component, either create an object or ensure that
    // an object by that name already exists.
    var container = core.globalNamespace;
    for(var i = 0; i < parts.length; i++) {
        var part = parts[i];
        // If there is no property of container with this name, create
        // an empty object.
        if (!container[part]) container[part] = {};
        else if (typeof container[part] != "object") {
            // If there is already a property, make sure it is an object
            var n = parts.slice(0,i).join('.');
            throw new Error(n + " already exists and is not an object");
        }
        container = container[part];
    }

    // The last container traversed above is the namespace we need.
    var namespace = container;
    
    // It is an error to define a namespace twice. It is okay if our
    // namespace object already exists, but it must not already have a
    // NAME property defined.
    if (namespace.NAME) throw new Error("Then namespace '"+name+"' is already defined");

    // Initialize name and version fields of the namespace
    namespace.NAME = name;

	if(typeof init === 'object')
	{
		for(var o in init)
		{
			if(o != "NAME")
				namespace[o] = init[o];
		}
	}
    // Register this namespace in the map of all modules
    core.modules[name] = namespace;

    // Return the namespace object to the caller
    return namespace;
}
/***
 * Test whether the module with the specified name has been defined.
 * Returns true if it is defined and false otherwise.
 */
core.isDefined = function(name) {
    return name in core.modules;
};

/***
 * This function is to register a 3rd part object, etc jQuery
 * to call 'get' function to get the object which is registerd  
 */
core.registerObject = function(name, obj) {
	
    if (name in core.REGISTER) {
        throw new Error("The name: '" + name + "' is registered");
    }
    
    core.REGISTER[name] = obj;
};

/***
 * This function is to get a 3rd part object 
 */
core.get = function(name)
{
	if (!(name in core.REGISTER)) {
        throw new Error("The object '" + name + "' is not defined");
    }
	
	return core.REGISTER[name];
};

core.abort = function()
{
	ajaxQuery.abort();
};

/***
 * This function imports symbols from a specified module.  By default, it
 * imports them into the global namespace, but you may specify a different
 * destination as the second argument.
 *
 * If no symbols are explicitly specified, the symbols in the EXPORT
 * array of the module will be imported. If no such array is defined,
 * and no EXPORT_OK is defined, all symbols from the module will be imported.
 *
 * To import an explicitly specified set of symbols, pass their names as
 * arguments after the module and the optional destination namespace. If the
 * modules defines an EXPORT or EXPORT_OK array, symbols will be imported
 * only if they are listed in one of those arrays.
 */
core.require = function(from) 
{
    // Make sure that the module is correctly specified. We expect the
    // module's namespace object but will try with a string, too
    if (typeof from == "string") from = core.modules[from];
    if (!from || typeof from != "object")
        throw new Error("core.import( ): " +
                        "namespace object required");

    // The source namespace may be followed by an optional destination
    // namespace and the names of one or more symbols to import;
    var to = core.globalNamespace; // Default destination
    var symbols = [];                // No symbols by default
    var firstsymbol = 1;             // Index in arguments of first symbol name

    // See if a destination namespace is specified
    if (arguments.length > 1 && typeof arguments[1] == "object") {
        if (arguments[1] != null) to = arguments[1];
        firstsymbol = 2;
    }

    // Now get the list of specified symbols
    for(var a = firstsymbol; a < arguments.length; a++)
        symbols.push(arguments[a]);

    // If we were not passed any symbols to import, import a set defined
    // by the module, or just import all of them.
    if (symbols.length == 0) {
        // If the module defines an EXPORT array, import
        // the symbols in that array.
        if (from.EXPORT) {
            for(var i = 0; i < from.EXPORT.length; i++) {
                var s = from.EXPORT[i];
                to[s] = from[s];
            }
            return;
        }
        // Otherwise if the modules does not define an EXPORT_OK array,
        // just import everything in the module's namespace
        else if (!from.EXPORT_OK) {
            for(s in from) to[s] = from[s];
            return;
        }
    }

    // If we get here, we have an explicitly specified array of symbols
    // to import. If the namespace defines EXPORT and/or EXPORT_OK arrays,
    // ensure that each symbol is listed before importing it.
    // Throw an error if a requested symbol does not exist or if
    // it is not allowed to be exported.
    var allowed;
    if (from.EXPORT || from.EXPORT_OK) {
        allowed = {};
        // Copy allowed symbols from arrays to properties of an object.
        // This allows us to test for an allowed symbol more efficiently.
        if (from.EXPORT)
            for(var i = 0; i < from.EXPORT.length; i++)
                allowed[from.EXPORT[i]] = true;
        if (from.EXPORT_OK)
            for(var i = 0; i < from.EXPORT_OK.length; i++)
                allowed[from.EXPORT_OK[i]] = true;
    }

    // Import the symbols
    for(var i = 0; i < symbols.length; i++) {
        var s = symbols[i];              // The name of the symbol to import
        if (!(s in from))                // Make sure it exists
            throw new Error("core.require( ): symbol " + s +
                            " is not defined");
        if (allowed && !(s in allowed))  // Make sure it is a public symbol
            throw new Error("core.require( ): symbol " + s +
                            " is not public and cannot be imported.");
        to[s] = from[s];                 // Import it
    }
}

core.bgProcess = false;
core.async = true;
core.request = function(url, data, success_callback, fail_callback, defaultType, contentType,bgProcess,proData)
{
	//core.printTime('data:' + (typeof data == 'object' ? data.xml : data));
	//core._debug('url:' + url);
	//core._debug('data:' + (typeof data == 'object' ? data.xml : data));
	//core._debug('proData:' + (typeof proData == 'object' ? data.xml : proData));
	if(typeof defaultType === "undefined")
		defaultType = AJAX_JSON;
		
	if(typeof contentType === "undefined")
		contentType = AJAX_JSON;

	var postData;
	if($.isArray(data))
		postData = data.join('&');
	else
		postData = data;
		
	if(typeof bgProcess == 'boolean')
		core.bgProcess = bgProcess;

	if(!core.bgProcess)
		core.blockUI();
		
	ajaxQuery.add({
	//$.ajax({
		url: url,
		type: "POST",
		dataType: defaultType,
		contentType: contentType,
		data: postData,
		async:core.async,
		cache:false, 
		ifModified :true,
		timeout: default_api_timeout,
		processData: (typeof proData === 'boolean')?proData:false,  //Default:false, to avoid changing the datatype from xml to string by jquery. 
		statusCode:{
			"404": function(){ /*alert(url + " page not found");*/ }
		},
		success: function(data, textStatus, jqXHR){
			//if(!core.bgProcess)
			//	core.unblockUI();
			success_callback.call(this, data, textStatus, jqXHR);
		},
		error: function(jqXHR, textStatus, thrown){
			if(!core.bgProcess)
				core.unblockUI();
			
			if(textStatus == 'timeout')
			{
				t4u.showMessage(multiLang.search('Wireless Media Drive is unavailable temporarily. Please try again later'));
				//core.printTime('Timeout');
			}
			/*
			if(textStatus != 'abort')
			{
				core.printTime('data:' + (typeof data == 'object' ? data.xml : data));
				core._debug('url:' + url);
				core._debug('data:' + (typeof data == 'object' ? data.xml : data));
				core._debug('proData:' + (typeof proData == 'object' ? data.xml : proData));
				core._debug('textStatus:' + textStatus);
			}
			*/
			fail_callback.call(this, jqXHR, textStatus, thrown);
		}
	});	
}

core.requestFail = function(jqXHR, textStatus, thrown){t4u._error(textStatus); t4u.bgProcess = false;}

core.printall = function(obj, show){
	var msg = [];
	for(var o in obj)
		msg.push(o + ":[" + typeof obj[o] + "]");
		
	if(show === UNDEFINED || show)
		alert(msg.join('\n'));
		
	return msg;
}

core.printTime = function(step){
	var date = new Date();
	var result = date.getFullYear() + '/' + (date.getMonth() + 1) + '/' + date.getDate() + ' ' + date.getHours() + ':' + date.getMinutes() + ':' + date.getSeconds();
	core._debug(step + '   ' + result)
}

function containChinese(str) {
    return /[\u4e00-\u9a05]/.test(str);
}

function isDoubleByte(str) {
    for (var i = 0, n = str.length; i < n; i++) {
        if (str.charCodeAt( i ) > 255) { return true; }
    }
    return false;
}

core.encodeURI = function(value){
			return encodeURIComponent(value);
	}

core.decodeURI = function(value){
	return decodeURIComponent(value);
}

core._error = function(message){core._debug('<span style="color: #f00">error:</span>'+message); }
core._warn = function(message){core._debug('<span style="color: #FAAC58">warn:</span>'+message); }
core._debug = function(message)
{
	if(typeof __DEBUG__ === 'boolean' && __DEBUG__)
	{
		var debug = $('#debug');
		if(debug.length == 0)
		{
			debug = $('<div id="debug" style="position: absolute; left:0; top:0, z-index: 9999; background-color: #000; color: #fff; font-size: 10px; line-height: 13px;"></div>');
			debug.appendTo('body');
			debug.css({left:0, top: 0});
			debug.draggable({containment: 'body'});
			debug.css('zIndex', '9999');
			debug.dblclick(function(){$(this).text('')});
		}
		debug.html(debug.html() + message + '<br />');
	}
}
core._console = function(message)
{
	if(typeof __DEBUG__ === 'boolean' && __DEBUG__ && typeof console != 'undefined')
	{
		console.log(message);	
	}
}

/***
	Author:Tim
	Desc:the function will show the dialog which contain the message.
	Parameter: the type of obj can be string or JSON(object).
			   JSON format example: var obj={title:'Error(default value)',message:'msg,which can be html'
												   ,buttons:{
														"Delete all items": function() {
															$( this ).dialog( "close" );
															return true;
														},
														Cancel: function() {
															$( this ).dialog( "close" );
														}
													},option:{
														resizable: false,
														modal: true
													}}
*/
core.showMessage = function(obj){
	
	if($('#alertDialog').length>0)
		$('#alertDialog').remove();
	
		
	var title=multiLang.search( 'Message');
	var message='';
	var option={resizable: false,modal: true,  position: 'center', height: 'auto'
					,create: function(event, ui) {
					  $("body").css({ overflow: 'hidden' });
					}
					,beforeClose: function(event, ui) {
					  $("body").css({ overflow: 'inherit' });
					}}; 
	var buttons={"OK": function() {$( this ).dialog( "close" );}};
		
	if(typeof obj=='string')
		message=obj;
	else{
		if(typeof obj.title!='undefined')
			title=obj.title;
		if(typeof obj.message!='undefined')
			message=obj.message;
		if(typeof obj.option!='undefined')
			option=obj.option;
		if(typeof obj.buttons!='undefined')
			buttons=obj.buttons;
	}
	
	option.buttons=buttons; //add the buttons to the option

	$('body').append('<div id="alertDialog" title="'+title+'" style="display:none;">'+message+'</div>');
	$('#alertDialog').dialog(option);
};

//var data={title:'title',message:'Do you want to delete the file?',txtYes:'OK',txtNo:'Cancel',eventbtnYes:function(){alert(123)}};
core.confirm = function(data){
	
	var button=new Object();
	
	button[data.txtYes] = function(){
		data.eventbtnYes.call(this);
		$( this ).dialog( "close" );
	};
	button[data.txtNo] = function() {
		$( this ).dialog( "close" );
	};
	
	var obj={title:data.title,message:data.message
			   ,buttons:button
			   ,option:{
					resizable: false,
					modal: true	,  
					position: 'center', 
					height: 'auto',
					create: function(event, ui) {
					  $("body").css({ overflow: 'hidden' })
					},
					beforeClose: function(event, ui) {
					  $("body").css({ overflow: 'inherit' })
					}
			}};
	core.showMessage(obj);
};

core.growl = function(title, message){
	$.growlUI(title, message);
};
core.blockUI = function(specified, specifiedCss){
	var def = '<img src="/data/images/ajax-loader.gif" />';
	if(typeof specified !== 'undefined')
		def = specified;
	else
	{
		return;
	}
	$.blockUI({message: def, css: specifiedCss, focusInput: false});
	$('.blockUI.blockMsg').not('.blockElement').center();
}

core.blockUIMsg = function(){
	var id = '#div-block-msg';
	$('#btnMessageCancel').hide();
	$('#hTitle').text(multiLang.search('In progress...'));
	core.blockUI($(id));
	$('#btnMessageCancel').click(function(){
		//$('#btnMessageCancel').off('click');
		$.unblockUI();
		core.abort();
	});
}

core.unblockUI = function(){
	$.unblockUI();
}

core.blockUIWithTitleAndCallback = function(title, callback) {
	var id = '#div-block-msg';
	$('#btnMessageCancel').show();
	$('#hTitle').text(title + '...');
	core.blockUI($(id));
	$('#btnMessageCancel').click(function(){
		$.unblockUI();
		if(typeof callback == 'function')
		{
			callback();
		}
	});
}

core.elementBlockUI = function(title, callback){
	var id = '.top';
	$('#hTitle2').text(title + '...');
	$(id).block({ message: $('#div-short-block-msg'), baseZ: 9999, showOverlay:false}); 
	$('#btnMessageCancel2').click(function(){
		$(id).unblock();
		if(typeof callback == 'function')
		{
			callback();
		}
	});
};

core.elementUnblockUI = function(){
	var id = '.top';
	$(id).unblock(); 
};

//here should add version to tail of js file name, for example: aa.js?20120816
core.loadJs = function(src)
{
	var script = document.createElement('script');
	script.setAttribute("type","text/javascript");
	script.setAttribute("src", src + "?20120816");
	$('head').append(script);
}

//Load JS by pure javascript.
core.loadJsByOnlyJs = function(src)
{
	var ele = document.createElement('script');
    ele.src = src;
    document.getElementsByTagName('body')[0].appendChild(ele);
}

//format string like C#.if the  arguments is float 1.0,then it will become 1
core.format = function()
{
    if( arguments.length == 0 )
		return null; 
    
    var str = arguments[0]; 
    for(var i=1;i<arguments.length;i++)
    {
        var re = new RegExp('\\{' + (i-1) + '\\}','gm');
        str = str.replace(re, arguments[i]);
    }
    return str;
}

core.detectMobileDev = function()
{
	var isAndroid = navigator.userAgent.match(/Android/i);
	var isiPadOriPhone = navigator.userAgent.match(/iPad/i) || navigator.userAgent.match(/iPhone/i);
	__MOBILE__ = navigator.userAgent.match(/Android/i) ||
			     navigator.userAgent.match(/webOS/i) ||
				 navigator.userAgent.match(/iPhone/i) ||
				 navigator.userAgent.match(/iPad/i);
	if(__MOBILE__)
	{
		$(function(){
			//t4u._warn('mobile device is detected');
		});
	}
	try
	{
		if(isAndroid)
		{
			var msg=multiLang.search('Please download the SanDisk Wireless Media Drive app for more functionality from your Android marketplace');
			//t4u.showMessage(msg);
			alert(msg);
		}
		else if(isiPadOriPhone)
		{
			setTimeout(function()
				{
					if(navigator.userAgent.match(/iPhone/i))
					{
						$('#btnMediaDrive').attr('href', 'https://itunes.apple.com/us/app/sandisk-connect-wireless-media/id646153884?mt=8');
						$('#btnAppStore').attr('href', 'https://itunes.apple.com/us/app/sandisk-connect-wireless-media/id646153884?mt=8');
					}
					else
					{
						$('#btnMediaDrive').attr('href', 'https://itunes.apple.com/us/app/sandisk-connect-wireless-media/id646168685?mt=8');
						$('#btnAppStore').attr('href', 'https://itunes.apple.com/us/app/sandisk-connect-wireless-media/id646168685?mt=8');
					}
					core.blockUI($('#div-ios-msg'));
				}, 1000);				
			//var msg=multiLang.search('Please download the SanDisk Wireless Media Drive app for more functionality from the app store');
			//t4u.showMessage(msg);
			//alert(msg);
		}
	}
	catch(e)
	{
		alert('error');
	}
}

core.Collection = function() {
	var data = [];

	this.add = function(items) {
		if ($.isArray(items))
		{
			for (var i = 0; i < items.length; i++)
				data.push(items[i]);
		}
		else
			data.push(items);
	}

	this.get = function(index) {
		if ($.isNumeric(index) && index >= 0 && index < data.length)
			return data[index];
		else
			return null;
	}

	this.size = function() {
		return data.length;
	}

	this.clear = function() {
		data.length = 0;
	}

	this.sort = function(field, reverse) {
		data.sort(sort_by(field, reverse));
		return true;
	}

	/***
	*	Author:Tim
	*	Desc:The JSON sort expression.
	*	@param {string} field - the field we want to sort.
	*	@param {bool} reverse - Is reverse? 
	*	@param {object} primer - To do something before to sort.
	*	@return {object} - the sort expression.
	*/
	function sort_by(field, reverse, primer){
	   var key = primer ? function (x) {return primer(x[field])} : function (x) { return x[field]; };

	   return function (a,b) {
		   var A = key(a), B = key(b);
		   return ((A < B) ? -1 :
				   (A > B) ? +1 : 0) * [-1,1][+!!reverse];                  
	   }
	}
}

$.fn.center = function () {
    this.css("position","absolute");
    this.css("top", ($(window).height() - this.height()) / 2+$(window).scrollTop() + "px");
    this.css("left", ( $(window).width() - this.width() ) / 2+$(window).scrollLeft() + "px");
    return this;
}

//callback: {onFound, onDone}
function MediaFinder (type, callback) {

	this.init = function(type, callback) {
		this.type = type;
		this.callback = callback;
		this.collection = new core.Collection();	
	}
	this.init(type, callback);
	
	var self = this;

	function readData(keyword, index, length) {
		self.keyword = keyword;
		t4u.nimbusApi.fileExplorer.searchMediaList(function(data) {
			if(t4u.nimbusApi.getErrorCode(data) == 0)
			{
				var items = t4u.nimbusApi.getRemoteItems(data);
				if (typeof items[0].path === 'undefined')
				{
					self.__triggerDone();
					return;
				}

				self.collection.add(items);
				self.__triggerFound(data, index, length);

				if (items.length == length)
					readData(keyword, index + length, length);
				else
					self.__triggerDone();
			}
		},
		function(data){
		}, {
			'mediaType': {
				'start': index,
				'count': length,
				'type': self.type
			}
		}, {
			filter: {
				'field': 'title',
				'value': self.keyword
			}
		});
	}

	this.__triggerDone = function() {
		if ($.isPlainObject(this.callback) && $.isFunction(this.callback.onDone))
			this.callback.onDone.call(this);
	}

	this.__triggerFound = function(data, index, length) {
		if ($.isPlainObject(this.callback) && $.isFunction(this.callback.onFound))
			this.callback.onFound.call(this, data, index, length);
	}

	this.find = readData;
	this.clear = function() {
		this.collection.clear();
	}
}
core.MediaFinder = MediaFinder;

function FolderFinder(callback){
	this.init('folder', callback);
};
FolderFinder.prototype = new MediaFinder();
FolderFinder.prototype.find = function(keyword, index, length) {
	var self = this;
	t4u.nimbusApi.fileExplorer.find(function(data) {
		if(t4u.nimbusApi.getErrorCode(data) == 0)
		{
			var items = t4u.nimbusApi.getRemoteItems(data);
			if (typeof items[0].path === 'undefined')
			{
				self.__triggerDone();
				return;
			}

			self.collection.add(items);
			self.__triggerFound(data, index, length);

			if (items.length == length)
				self.find(keyword, index + length, length);
			else
				self.__triggerDone();
		}
	}, 
	function(data) {
	},
	{
		start: index,
		count: length,
		path: '/'
	}, keyword);
}
core.FolderFinder = FolderFinder;

var PropertyManager = function (key, value) {
	var delimiter = "\n",
		props = {},
		isSaving = false,
		pending = false,
		self = this;

	this.set = function(key, value) {
		if (isSaving)
			pending = true;
		props[key] = value;

		return this;
	};

	this.get = function(key) {
		return typeof props[key] === 'undefined' ? null : props[key];
	};

	this.save = function(completeCallback) {
		toCookie(completeCallback);
		// toServer(completeCallback);
	};

	function fromCookie(completeCallback)
	{
		var value = $.cookie('properties');
		core.notShowHint = $.cookie('NoVideoHint');
		if (value != null)
		{
			var properties = value.split(',');
			for (var i = 0; i < properties.length; i++)
			{
				var key = properties[i];
				props[key] = $.cookie(key);
			}
		}
		clearCookie();

		if ($.isFunction(completeCallback))
			completeCallback.call(self);
	}

	function toCookie(completeCallback)
	{
		var keys = [];
		for (var key in props)
		{
			if (typeof key !== 'undefined')
			{
				var name = new String(key);
				if (name != "")
				{
					keys.push(name);
					$.cookie(name, props[key], { expires: 365 * 10 });
				}
			}
		}

		$.cookie('properties', keys.join(','), { expires: 365 * 10 });
	}

	function clearCookie()
	{
		if (document.cookie)
		{
			var cookies = document.cookie.split(';');

			for (var i = 0; i < cookies.length; i++)
			{
				var cookie = cookies[i];
				var eqPos = cookie.indexOf('=');
				var name = eqPos > -1 ? cookie.substr(0, eqPos) : cookie;
				document.cookie = name + '=;expires=Thu, 01 Jan 1970 00:00:00 GMT';
			}
		}
	}

	function fromServer(completeCallback)
	{
		$.ajax({
			url: '/storage/upload/.properties', 
			dataType: 'text/plain',
			complete: function(jqXHR) {
				if (jqXHR && jqXHR.status == 200)
				{
					var data = jqXHR.responseText;
					if (typeof data === 'string')
					{
						var p = data.split('\n');
						for (var i = 0; i < p.length; i++)
						{
							var kv = p[i].split('=', 2);
							props[kv[0]] = kv[1];
						}
					}
				}
				if ($.isFunction(completeCallback))
					completeCallback.call(self);
			}
		});
	}

	function toServer(completeCallback) 
	{
		isSaving = true;
		var data = [];
		for (var key in props)
			data.push(key + '=' + props[key]);

		var boundary = '-----------------------------' + (Math.floor(Math.random() * Math.pow(10, 8)));
	    var postData = [];
		postData.push('--' + boundary);
		postData.push('Content-Disposition: form-data; name="userfile"; filename=".properties"');
		postData.push('Content-Type: text/plain');
		postData.push('');
		postData.push(data.join(delimiter));
		var dataString = postData.join('\r\n');
		var waittime = Math.ceil(dataString.length / 51200.0) * 2;

		$.ajax({
			headers: {
				'Content-Type': 'multipart/form-data; boundary=' + boundary
			},
			url: '/NimbusAPI/upload?ow',
			type: "POST",
			data: dataString,
			processData: false,
			async: true,
			timeout: waittime,//i don't know why server stuck, set timeout to close connection
			beforeSend: function(xhrObj) {
				// xhrObj.setRequestHeader();
			},
			complete: function(jqXHR, textStatus) {
				isSaving = false;
				if (pending)
				{
					setTimeout(function() {
						pending = false;
						self.save();
					}, 200);
					return;
				}

				if ($.isFunction(completeCallback))
					completeCallback.call(this, jqXHR, textStatus);
			}
		});
	}

	this.load = function(completeCallback) {
		fromCookie(completeCallback);
		// fromServer(completeCallback);
	};
}

var _pm = new PropertyManager();
core.properties = function(key, value, undefined) {
	if (arguments.length == 1)
		return _pm.get(key);
	else if (arguments.length > 1)
		return _pm.set(key, value);

	return _pm;
}
$(window).unload(function() {
	core.properties().save();
});

window.t4u = core;
//__detectMobileDev();

})(window);