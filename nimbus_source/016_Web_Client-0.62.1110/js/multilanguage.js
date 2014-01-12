(function(){

	/*** 
		Author:Tim
		Desc: To get the local language by navigator.
		@param nothing
		@return {object} - language code(lowercase).
	*/
	function userLang(){
	
		if(typeof(navigator.browserLanguage) != "undefined")     
				return navigator.browserLanguage.toLowerCase();
			
		if(typeof(navigator.language) != "undefined")     
			return navigator.language.toLowerCase();
			
		if(typeof(navigator.userLanguage) != "undefined")     
			return navigator.userLanguage.toLowerCase();
		
		if(typeof(navigator.systemLanguage) != "undefined")     
			return navigator.systemLanguage.toLowerCase();
		
	}
	
	/*** 
		Author:Tim
		Desc: To get the lang pack correspond with the local language.If we do not support this language,we will return the default language.
		@param nothing
		@return {object} - language pack url.
	*/
	function getLangUrl(){	
		
		if(typeof langUrl[userLang()]=='undefined')
			return langUrl[defaultLang];
	
		return langUrl[userLang()];
	}
	
	/*** 
		Author:Tim
		Desc: To get the system code which defined by us correspond with the local language.If we do not support this language,we will return the default language.
		@param nothing
		@return {object} - system code.
	*/
	function getLangVal(){
		
		if(typeof langVal[userLang()]=='undefined')
			return langVal[defaultLang];	
	
		return langVal[userLang()];
	}
	
	/*** 
		Author:Tim
		Desc: To load the local lang pack.
		@param nothing
		@return nothing
	*/
	function loadLangJs(){	
		t4u.loadJs(langDir+'/'+getLangUrl());
	}

	var multiLang = {	
		currentLang:'en',
		/*** 
			Author:Tim
			Desc: Initial the programe.
			@param nothing
			@return nothing
		*/
		init:function(){
			lang.run();
		},
		
		/*** 
			Author:Tim
			Desc: To detect the user language and to load the lang pack.
			@param nothing
			@return nothing
		*/
		auto:function(){
			loadLangJs();
			multiLang.currentLang = getLangVal();
			lang.change(multiLang.currentLang);
		},
		
		/*** 
			Author:Tim
			Desc: Let the user can change the language manually.If the input does not support, we will use the default language.
			@param {string} langStr - the language code.
			@return nothing
		*/
		manual:function(langStr){
			langStr=langStr.toLowerCase();
			
			if(typeof langVal[langStr]=='undefined')
				langStr=defaultLang;
			
			t4u.loadJs(langDir+'/'+langUrl[langStr]);
			
			multiLang.currentLang = langVal[langStr];
			lang.change(multiLang.currentLang);
		},
		
		/*** 
			Author:Tim
			Desc: To search the translation of the word. The function run after initial and auto(or manual).
			@param {string} wordStr - The word we want to search.
			@return {string} - The translation of the word.
		*/
		search:function(wordStr){
			var ret = langWordJson()[wordStr];
			if(typeof ret == 'undefined')
				ret = wordStr;
				
			return ret;
		}
	};
	
	window.multiLang = multiLang;
})(window);