;(function($){

	/*** 
		Author:Tim
		Desc: Initial the all value.
		@param nothing
		@return nothing
	*/
	function initial(){
	
		listView.isSetTitle=false;
		listView.isFirstEnter=true;
		listView.songNum=1;
		listView.counter=1;
	}

	/*** 
		Author:Tim
		Desc: To set the image above this table.(It is useless,if the customer do not want have the info)
		@param {string} pic - the thumbnail path.
		@param {string} artist - the artist name.
		@param {string} album - the album name.
		@param {string} title - the song name.
		@param {bool} forced - forced to change the image above this table.
		@return nothing 
	*/
	function setTitle(pic,artist,album,title,forced)
	{
		if((!listView.isSetTitle) || forced){
			var thisItem = $('#musicPhoto'),
			defaultThumbnailGenerator = $('.photoThumbnailGeneratorMargin');
			defaultThumbnailGenerator.remove();
			if(pic == 'undefined')
			{
				pic = album;
				thisItem.after(getNoThumbnailBG(pic, '144x144'));
				$('.photoThumbnailGeneratorMargin').attr('style','width: 144px; height: 144px;left:-27px; top:-2px;');
				thisItem.hide();
			}
			else
			{				
				thisItem.show();
				thisItem.attr('src', pic);
			}
			$('#musicArtist').text(artist);
			$('#musicAlbumSong').text(album);;
			$('#musicSongName').text(title);
			
			listView.isSetTitle=true;
			listView.isFirstEnter=false;
			scrollbarInit();
		}
	}
		
	
	/*** 
		Author:Tim
		Desc: To create all html for the whole item.
		@param {object} data - API returns.
		@return {string} - the whole item html.
	*/
	function createRow(data){
		
		var time=audioView.convertSecond(data.duration);
		var tagStr=getAllJsonStr(data);
		var checkbox = '<td width="50"><input type="checkbox" class="cbSelection" value="{'
				+ '&quot;name&quot;: &quot;' + data.title + '&quot;,'
				+ '&quot;format&quot;: &quot;audio&quot;,'
				+ '&quot;isdir&quot;: false,'
				+ '&quot;path&quot;: &quot;' + data.path + '&quot;'
				+ '}" /></td>';
		
		// if(isMobile())
		// {
		// 	trTagStr= '<tr class="musicSong" ' + tagStr +'><td><input name="" type="checkbox" value="qsi" /><label></label></td>';
		// }
		// else
		// {
			var trTagStr = '<tr class="musicSong" ' + tagStr +'>';
		// }
		
		Num = '<td ><b>' + listView.songNum + '</b></td>';
		Song = '<td style="min-width: 124px">' + (typeof data.title == "string"?data.title: (typeof data.title == "object" ? data.title['#text'] : ""))  +'</td>';
		Artist = '<td style="min-width: 124px">' + (typeof data.artist == "string"?data.artist:(typeof data.artist == "object" ? (typeof data.artist['#text'] == 'undefined' ? multiLang.search('Unknown artist') : data.artist['#text']) : ""))  + '</td>';
		Time =  '<td>' + time+ '</td>' ;
		var arrow = '<td style="width: 35px">' + t4u.ui.arrowButton + '</td>';
		var result=trTagStr + checkbox + Num + Song + Artist + Time  + arrow + '</tr>'; 
		
		listView.songNum++;
		
		
		return result;
	
	}
	
	/*** 
		Author:Tim
		Desc: To create all html which we want to append.
		@param {object} data - API returns.
		@return {string} - the all html.
	*/
	function createAllHtml(data){
		
		var html='';
		
		if(typeof data.length=='undefined'){
		
			if(typeof data.duration=='undefined')
				return;
		
			html=createRow(data);		
		}else{
			for(var i=0;i<data.length;i++){
				html+=createRow(data[i]);		
			}
		}
		
		return html;
	}
	
	/*** 
		Author:Tim
		Desc: To append all html.
		@param {string} selectorStr - the selector we want to append.
		@param {object} data - API returns.
		@param {string} isAppend - Whether the data need to append.
		@return {string} - the all html.
	*/
	function appendAllHtml(selectorStr,data,isAppend){
		
		data=getObjEntry(data);
		
		if(typeof data["path"]!='undefined' || data.length < inifiniteScrollSize || (typeof data["path"]=='undefined' && typeof data.length=='undefined'  ) ){
			listView.isAllData=true;
		}
		
		
		var html=createAllHtml(data);
		if(!isAppend)
		{
			$(selectorStr +' tr').not(":first-child").remove();
			if(typeof html === 'undefined')
				$('#div-no-records-found-music').show();
		}
		if(!$('#div-no-records-found-music').is(':visible'))
		$(selectorStr).append( html );
		
	}
	
	
	/*** 
		Author:Tim
		Desc: To setup the list view.
		@example: dataJson=[{'field':'title','value':'getArtist'}]//it is option. selectorStr='#divMusicViewerList table' 
		@param {string} selectorStr - It is a table will be append data and remove original data.
		@param {object} data - API returns.
		@param {string} isAppend - Whether the data need to append.
		@return nothing
	*/
	function setup(selectorStr,dataJson,isAppend){
	
		var filter={'filter':[]};	
		if(typeof dataJson!='undefined'){
			filter=dataJson;	
		}
		
		audioView.showAlbumInfo(mode == modeCategory.artist || mode == modeCategory.album || mode == modeCategory.genre);
		
		t4u.nimbusApi.fileExplorer.getMediaList(function(data){
			if(t4u.nimbusApi.getErrorCode(data) == 0){			
				appendAllHtml(selectorStr,data,isAppend);
				showScrollbar(false);
			}
			listView.loadingDataFromServer(false);
			setFixedTitleNoData();
			setFixedTitle(selectorStr);
		},
		function(data){
			listView.loadingDataFromServer(false);
		},
		{'mediaType':[{'type':'music','start':listView.counter,'count':inifiniteScrollSize}],'sort':[{'order':'asc','field':'title'}]},filter);
	}
	
	/*** 
		Author:Steven
		Desc: To search the data
		@example: dataJson=[{'field':'title','value':'getArtist'}]//it is option. selectorStr='#divMusicViewerList table' 
		@param {string} selectorStr - It is a table will be append data and remove original data.
		@param {object} data - API returns.
		@param {string} isAppend - Whether the data need to append.
		@return nothing
	*/
	function search(selectorStr,dataJson,isAppend, keyword){
	
		var filter={'filter':[]};	
		if(typeof dataJson!='undefined'){
			filter=dataJson;	
		}
		
		
		t4u.nimbusApi.fileExplorer.searchMediaList(function(data){
				if(t4u.nimbusApi.getErrorCode(data) == 0){			
					appendAllHtml(selectorStr,data,isAppend);
				}
				listView.loadingDataFromServer(false);
				setFixedTitle(selectorStr);
			},
			function(data){
				listView.loadingDataFromServer(false);
			},{'mediaType':[{'type':'music','start':listView.counter,'count':inifiniteScrollSize}],'sort':[{'order':'asc','field':'title'}]},{
			filter: {
				'field': 'title',
				'value': keyword
			}
		});
	}
	
	var listView = {
		
		isSetTitle:false,
		isFirstEnter:false,
		isAllData:false,
		isLoading:false,
		setupFilter: '',
		songNum:1,
		counter:1,
		_selector:'',
		
		/*** 
			Author:Tim
			Desc:  To setup the list view. If isAppend is true,we won't initial. If isAllData is true, we won't do anything.
			@example: dataJson=[{'field':'title','value':'getArtist'}]//it is option. selectorStr='#divMusicViewerList table' 
			@param {string} selectorStr - It is a table will be append data and remove original data.
			@param {object} data - API returns.
			@param {string} isAppend - Whether the data need to append.
			@return nothing
		*/
		setup:function(selectorStr,dataJson,isAppend){
			this.loadingDataFromServer(true);
		
			if(listView.isAllData)
				return;
			
			if(typeof isAppend=='undefined')
				isAppend=false;
			
			if(!isAppend)
				listView.isFirstEnter=true;
			else
				listView.isFirstEnter=false;
				
			if(listView.isFirstEnter)
				initial();
			
			this._selector = selectorStr;		 
			
			this.setupFilter = '';
			setup(selectorStr,dataJson,isAppend);

			var $listContent = $('#divMusicViewerList .scrollDiv');
			$listContent.unbind();
			$listContent.bind('scroll', function() {
				if(mode != 'recent' && mode != 'playlist' && mode != 'nowPlayingList') { // recent & playlist items up to 30
					if($(this).scrollTop() + $(this).innerHeight() >= $(this)[0].scrollHeight*0.95) {
						audioView.listView.load($('#divMusicViewerList table'));
					}
				}
			})

		},
		
		/*** 
			Author:Tim
			Desc:  To setup the list view. If isAppend is true,we won't initial. If isAllData is true, we won't do anything.
			@example: dataJson=[{'field':'title','value':'getArtist'}]//it is option. selectorStr='#divMusicViewerList table' 
			@param {string} selectorStr - It is a table will be append data and remove original data.
			@param {object} data - API returns.
			@param {string} isAppend - Whether the data need to append.
			@param {string} keyword - Keyword to search.
			@return nothing
		*/
		search:function(selectorStr,dataJson,isAppend, keyword){
		
			if(listView.isAllData)
				return;
			
			if(typeof isAppend=='undefined')
				isAppend=false;
			
			if(!isAppend)
				listView.isFirstEnter=true;
			else
				listView.isFirstEnter=false;
				
			if(listView.isFirstEnter)
				initial();
			
			this._selector = selectorStr;		 
			this.setupFilter = keyword;
			search(selectorStr,dataJson,isAppend, keyword);

			var $listContent = $('#divMusicViewerList .scrollDiv');
			$listContent.unbind();
			$listContent.bind('scroll', function() {
				if(mode != 'recent' && mode != 'playlist' && mode != 'nowPlayingList') { // recent & playlist items up to 30
					if($(this).scrollTop() + $(this).innerHeight() >= $(this)[0].scrollHeight*0.95) {
						audioView.listView.load($('#divMusicViewerList table'), true);
					}
				}
			})

		},
		
		/*** 
			Author:Tim
			Desc:  To setup the list view which only has the unikey
			@param {string} selectorStr - It is a table will be append data and remove original data.
			@param {object} data - the unikey array.
			@return nothing
		*/
		setupUnikeyView:function(selectorStr,data){
			
			var html='';
			
			this._selector = selectorStr;
			
			for(var i=0;i<data.length;i++){
				
				var template={path:unescape(data[i]),unikey:unescape(data[i])};
				html+=createRow(template);
			
			}
			
			$(selectorStr +' tr').not(":first-child").remove();
			$(selectorStr).append( html );
		
		},
		
		/*** 
			Author:Tim
			Desc:  To set the image above this table.(It is useless,if the customer do not want have the info)
			@param {object} selector - the selector which contains the information.
			@return nothing
		*/
		changeTitlePic:function(selector){
			setTitle(selector.attr('thumbnail'),selector.attr('artist'),selector.attr('album'),selector.attr('title'),true)
		},
		
		load:function(selector, isSearch) {
			var filter = {'filter': []};
			if(audioView.filter != null)
			{
				filter = audioView.filter;
			}
			else if (this.setupFilter != '')
			{
				filter = {
					'filter': {
						'field': 'title',
						'value': this.setupFilter
					}
				};
			}
			t4u.bgProcess = true;
			if(!listView.isLoading) {
			    listView.isLoading = true;
				if(typeof isSearch != 'undefined')
				{
					t4u.nimbusApi.fileExplorer.searchMediaList(function(data){
							if(t4u.nimbusApi.getErrorCode(data) == 0){			
								appendAllHtml(selector,data, 1);
								setFixedTitle(selector);
							}
						listView.isLoading = false;
						},function(data){
						},
						{'mediaType':[{'type':'music','start':listView.songNum,'count':inifiniteScrollSize}],'sort':[{'order':'asc','field':'title'}]},filter);
				}
				else
				{
					t4u.nimbusApi.fileExplorer.getMediaList(function(data){
							if(t4u.nimbusApi.getErrorCode(data) == 0){			
								appendAllHtml(selector,data, 1);
								setFixedTitle(selector);
							}
						listView.isLoading = false;
						},function(data){
						},
						{'mediaType':[{'type':'music','start':listView.songNum,'count':inifiniteScrollSize}],'sort':[{'order':'asc','field':'title'}]},filter);
				}
			}
		},
		
		clear:function(){
			if(this._selector != '')
			{
				$(this._selector).html('');
			}
		},
		loadingDataFromServer: function(loading) {
			if(loading)
			{
				$('#div-dynamicLoading-music-list').show();
				$(this._selector).hide();
			}
			else
			{
				$('#div-dynamicLoading-music-list').hide();
				$(this._selector).show();
			}
		}
		
	};

	window.audioView.listView = listView;
})(jQuery);