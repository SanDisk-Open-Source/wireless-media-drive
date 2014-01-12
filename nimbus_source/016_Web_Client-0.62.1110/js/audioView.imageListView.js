;(function($){
	
	/*** 
		Author:Tim
		Desc: To generate the html which will be processed.
		@param {object} data - API returns.
		@param {string} dirStr - the dir.
		@return {string} - the img html. 
	*/
	function getLiImg(data,dirStr){
		
		var str='';
		var allTagStr=getAllJsonStr(data);	
		
		str='<img '+allTagStr+' class="pos3" process="" style="display:none;" /></a><img class="pos2" style="display:none;" />'+
			'<img class="pos1" style="display:none;" />';
			
		return str;
	}
	
	/*** 
		Author:Tim
		Desc: To create all html for the whole item.
		@param {object} data - API returns.
		@param {string} dirStr - the dir.
		@return {string} - the whole item html.
	*/
	function createImg(data,dirStr){
	
		var allTagStr='',
			allJsonValArr=[],
			defaultGeneratorStr='';
		
		allTagStr=getAllJsonStr(data);		
		
		var result = '';
		result = '<li ' + allTagStr + ' ><div class="controlAlbum">';
		result += '<a class="controlBlue" ' + allTagStr + ' href="javascript:void(0)" >';
		if(dirStr == 'artist')
			defaultGeneratorStr = data.artist;
		else if(dirStr == 'album')
			defaultGeneratorStr = data.album;
		else if(dirStr == 'genre')
			defaultGeneratorStr = data.genre;
		result += getNoThumbnailBG(defaultGeneratorStr, '144x144');
		result += getLiImg(data, dirStr);//三張圖
		result += '</div>';
		//result += '<input name="" type="checkbox" value="qqq" />';
		if(isMobile()){
			result += '<label>'+allJsonValArr[i]+'</label>';
		}
		else{
			var name = '';
			if(typeof data[dirStr] != 'string') {
				if(dirStr == 'artist')
					name = multiLang.search('Unknown artist');
				else if(dirStr == 'album')
					name = multiLang.search('Unknown album');
				else if(dirStr == 'genre')
					name = multiLang.search('Unknown genre');
			} else {
				name = data[dirStr];
			}
			
			result += '<div class="title" style="white-space: nowrap; height:20px;">'+showShortAlbumName(name)+'</div><span class="thumbnailShowRighClickArrow"></span>';
		}
		result += '</li>';
		return result;
	}
	
	function showShortAlbumName(albumName)
	{
		var newAlbumName = '<div style="width:144px;" class="shortInfor" title = "' + albumName + '">' + albumName + '</div>';
		return newAlbumName;
	}
	
	/*** 
		Author:Tim
		Desc: To create all html which we want to append.
		@param {object} data - API returns.
		@param {string} dirStr - the dir.
		@return {string} - the all html.
	*/
	function createAllHtml(data,dirStr){
	
		var html='';
		
		if(typeof data.length=='undefined'){
		
			if(getAllJsonStr(data)=='')
				return;
		
			html=createImg(data,dirStr);		
		}else{
			for(var i=0;i<data.length;i++){
				if(typeof data[i][dirStr] == "string" || typeof data[i][dirStr] == "number" || typeof data[i][dirStr] == "object") // 如果有值的話
				html+=createImg(data[i],dirStr);
			}
		}
		
		return html;
	}
	
	/*** 
		Author:Tim
		Desc: To append all html and to process all thumbnail.
		@param {string} selectorStr - the selector we want to append.
		@param {object} data - API returns.
		@param {string} dirStr - the dir.
		@return {string} - the all html.
	*/
	function appendAllHtml(selectorStr,data,dirStr){
		
		data=getObjEntry(data);
		
		if(typeof data[dirStr]!='undefined' || data.length < inifiniteScrollImgSize || (typeof data[dirStr]=='undefined' && typeof data.length=='undefined'  ) ){
			imageListView.isAllData=true;
		}
		
		var html=createAllHtml(data,dirStr)

		if(typeof data.length=='undefined'){
			// 如果album, artist, genre只一個項目則hide下個畫面再trigger click
			$('.musicSong').remove();
			$('#divMusicViewerList-List').hide();
			audioView.albumCount = 1;
		}
		else
			audioView.albumCount = data.length;
		
		// if(!imageListView.isAppend)
		// 	$(selectorStr).children().remove();
			
		$(selectorStr).append( html );
		$('.pos3').trigger('loadthumbnail');
				
		/*
		if(typeof data.length=='undefined'){
			$('.controlAlbum').click();
			$('#divMusicViewerList-List').show();
		}		
		*/
		
		imageListView.isLoading = false;
	}
	
	/*** 
		Author:Tim
		Desc: To setup this image list view.If the filter is undefined,we will set the the default filter(no any data).
		@param {string} selectorStr - the selector we want to append.
		@param {string} dirStr - the dir.
		@param {object} filter - the filter of search.
		@return nothing.
	*/
	function setup(selectorStr,dirStr,filter){
		imageListView.isLoading = true;
		t4u.bgProcess = true;
		if(typeof filter=='undefined')
			filter={filter:[]};
		if(mode == modeCategory.genre)
			audioView.genreFilter = filter;
		t4u.nimbusApi.fileExplorer.getCategoryList(
			function(data){
				if(t4u.nimbusApi.getErrorCode(data) == 0)
					appendAllHtml(selectorStr,data,dirStr)
				imageListView.loadingDataFromServer(false);
			},
			function(data){
				imageListView.loadingDataFromServer(false);
			},
			{'category':[{'dir':dirStr}],'mediaType':[{'start':imageListView.counter,'count':inifiniteScrollImgSize,'type':'music'}]
			,'sort':[{'order':'asc','field':dirStr}]},filter
		);
	}
	
	var imageListView = {
		counter:1,
		isAppend:false,
		isAllData:false,
		_selector:'',
		isLoading: false,
		
		/*** 
			Author:Tim
			Desc: To setup this image list view.If isAllData is true,we won't do anything.
			@param {string} selectorStr - the selector we want to append.
			@param {string} dirStr - the dir.
			@param {object} filter - the filter of search.
			@return nothing.
		*/
		setup:function(selectorStr,dirStr,filter){
			t4u.abort();
			this.loadingDataFromServer(true);
			this.isAllData = false;
			this.counter = 1;
			_selector = selectorStr;
			$(selectorStr).children().remove();
			setup(selectorStr,dirStr,filter);
			t4u.scroll.bind($('#content'), function() {
				if (imageListView.isLoading)
					return;
				if (imageListView.isAllData)
				{
					t4u.scroll.unbind();
					return;
				}
				imageListView.counter += inifiniteScrollImgSize;
				setup(selectorStr,dirStr,filter);			
			});
			showScrollbar(true);
		},
		clear:function()
		{
			if(this._selector != '')
			{
				$(this._selector).html('');
			}
		},
		loadingDataFromServer: function(loading) {
			if(loading)
			{
				$('#div-dynamicLoading-music-images').show();
				$(this._selector).hide();
			}
			else
			{
				$('#div-dynamicLoading-music-images').hide();
				$(this._selector).show();
			}
		}
	};

	window.audioView.imageListView = imageListView;
})(jQuery);