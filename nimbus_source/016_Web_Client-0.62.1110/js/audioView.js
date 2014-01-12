;(function($){
	
	/*** 
		Author:Tim
		Desc: All selector strings in audioView.
	*/
	var cssSelector={
		btnRecent:'#getting-Recent',
		btnArtist:'#music_button_artist',
		btnAlbum:'#music_button_albums',
		btnAllSong:'#getting-Songs',
		btnPl:'#getting-Playlists',
		btnGenre:'#music_button_genre',
		btnNowPl:'#getting-NowPlayinglists',
		panelListView:'#divMusicViewerList table',
		panelImageListView:'.musicAlbumsListItems',
		panelPlView:'#divMusicViewerMusicList table',
		panelPlDetailView:'#divMusicViewerCount table',
		panelMainListView:'#divMusicViewerList',
		panelMainImageListView:'#divMusicViewerPic',
		panelMainPlView:'#divMusicViewerMusicList',
		panelMainPlDetailView:'#divMusicViewerCount',
		panelbtnMusic:'#music-tabs',
		btnSearch:'#music_search_result'
	
	};
	
	/*** 
		Author:Tim
		Desc: To get the unikey of selector.
		@param {object} selector - It has the attributes path or it will return null.
		@return {string} -  unikey
	*/
	function getUnikeyStr(selector){
		return selector.attr('path');		
	}
	
	
	/*** 
		Author:Tim
		Desc: To play these songs after searching media.
		@param {object} selector - It has the some attributes we need.(It does not contain the unikey.)
		@return nothing
	*/
	function playAllSongsInSearch(selector){
		
		/* 這個callback似乎跟本沒用到
		var func=function(data){
			judgeThumbnail(data.thumbnail,data);
			audioPlaylist.play(data);
		}
		*/
		audioView.getAllSongsBySearch(selector);
	}
	
	
	/*** 
		Author:Tim
		Desc: To play these songs after searching media or directly playing.
		@param {object} selector - It has the some attributes we need.
		@return nothing
	*/
	function playSong(selector){
	
		if(typeof(selector)=='undefined')
			selector=$('[tag="clicked"]:visible');
	
		if(typeof selector.attr('path')!='undefined'){
			audioPlaylist.play(audioView.getSongJson(selector),getUnikeyStr(selector));
			return;
		}else{
			playAllSongsInSearch(selector);		
		}
	
	}
	
	/*** 
		Author:Tim
		Desc: To get the filter in the selector attribute.
		@param {object} selector - It may has the some attributes we need.
		@param {string} attr - an attribute string.
		@return {object} - the filter object
	*/
	function getFilterInAttr(selector,attr){
		
		var template={field:'',value: '' };
		
		template.field=attr;	
		
		if(typeof selector.attr(attr) == 'string'){
			template.value=selector.attr(attr);
		}
		
		return template;
	}
	
	/*** 
		Author:Tim
		Desc: To create the row in the Main playlist view.
		@param {object} dataList - the value of this cookie.
		@param {int} attr - the unique serial number.
		@param {string} cookieName - the cookie name.
		@return {string} - the html of this row.
	*/
	function createPlRow(dataList,i,cookieName){
	
		var result='<tr cookiename="'+cookieName+'" pllength="'+dataList.songs.length+'" plname="'+unescape(dataList.name)+'" class="musicSong">';
		
		if(isMobile())
		{
			result += '<td><input name="" type="checkbox" value="qsi" /><label></label></td>';
		}
		else
		{
			result += '<td></td>';
		}
		result += ('<td><b>' + (i+1) + '</b></td>');
		result += ('<td>' + unescape(dataList.name) + '</td>');
		result += ('<td>' + dataList.songs.length + '<span class="listShowRighClickArrow"></span>'+'</td></tr>');
	
		return result;
	}
	
	/*
		顯示目前正在播放的歌曲
	*/
	function showCurrentPlayingSong()
	{
		if(mode != modeCategory.nowPlayingList)
			return;
		if(AudioPlayer.isplaying || AudioPlayer.isPaused)
		{
			var index = AudioPlayer.playindex;
			if(AudioPlayer.playlist.length == 1)
				index = 0;
				
				
			$("#divMusicViewerList table tr").not(":first-child").removeClass("nowPlaying");
			$("#divMusicViewerList table tr").not(":first-child").children().removeClass();
			$("#divMusicViewerList table tr").not(":first-child").children().children().show();
			$("#divMusicViewerList table tr").trigger('mouseleave');
			
			$(".addGrayArrow02").remove();
			var playingRow = $("#divMusicViewerList table tr").not(":first-child").eq(index);
			playingRow.addClass("nowPlaying");
			if(!(playingRow.attr('enter') == 'enter'))
				playingRow.css({'background':'#33b5e5'});
			playingRow.children().eq(1).addClass("addGrayArrow");
			playingRow.children().eq(1).width(20);
			playingRow.children().eq(1).children().hide();
			playingRow.children().eq(1).append("<div class='addGrayArrow02'></div>");	

			$('#divMusicViewerList-List tr.musicSong').each(function(i) {
				if(i != index)
					$(this).find('td:eq(1)').not('.addGrayArrow').find('b').text(i+1);
			});			
		}
	}
	
	var audioView = {
		filter:null,
		genreFilter:null,
		dirStr:null,		
		timer:null,
		albumCount:1,//專輯的數目，如果是1，而且是在Genre或是Artist則跳到最下面那層，Back的時候，直接回到最上面，否則跳到上一層
		playlistIndex:0,//存放目前是Playlist的第幾個，從0開始
		moreSongByThisArtistInPlaylist:false,//More song by this artist，如果是在Playlist，標題的對齊要用另外一種
		
		/*** 
			Author:Tim
			Desc: To delete songs and remove the songs in the playlist.
			@param {object} selector - the item user want to delete.
			@return nothing
		*/
		/*** 
			Author:Tim
			Desc: To get the data of selector that audio player needs JSON.
			@param {object} selector - It has the attributes
			@return {string} -  audio player needs JSON
		*/
		getSongJson:function(selector){


		    var dataJson={albumname:"Kalimba",songname:'Kalimba',albumphoto:null,artist:''};
		    dataJson.albumname=selector.attr('album');
		    dataJson.songname=selector.attr('title');
		    dataJson.path=selector.attr('path');

		    var thumbnail=selector.attr('thumbnail');
			$('.albumback').hide();
		    judgeThumbnail(thumbnail,dataJson);
		    dataJson.artist=selector.attr('artist');
		    //dataJson[selector.attr('path').split('.')[selector.attr('path').split('.').length-1]]=t4u.encodeURI(selector.attr('path')) + '?t=' + ( new Date() ).getTime();
			dataJson[selector.attr('path').split('.')[selector.attr('path').split('.').length-1]]=t4u.encodeURI(selector.attr('path'));

			dataJson['path']=selector.attr('path');

		    return dataJson;
		},
		/*** 
			Author:Tim
			Desc: To search media by the selector which contain something filter.
			@param {object} selector - It has the some attributes we need.
			@param {object} func - callback function.
			@return nothing
		*/
		getAllSongsBySearch:function(selector, updatePlaylistOnly){
		      var func=function(data){
			  judgeThumbnail(data.thumbnail,data);
			  if(updatePlaylistOnly)
			      AudioPlayer.playlist.push(data);
			  else
			      audioPlaylist.play(data);
		      }
		    var filterJson=audioView.getAllFilterInAttr(selector);
		    audioView.searchMedia(filterJson,func);
		},
		deleteAudioInImageList:function(selector){
			
			var filter=audioView.getAllFilterInAttr(selector);
			if(filter.length==0){
				return;
			}
			
			var func=function(data){
				playlist.removeSong(data.path);
				t4u.nimbusApi.fileExplorer.deletePath(function(data){ }, function(data){ }, {forced:false,path:""+data.path});
			}
			
			audioView.searchMedia(filter,func);
		
		},
	
		/*** 
			Author:Tim
			Desc: To play songs in the folder view.
			@param {string} pathStr - the file path.
			@return nothing
		*/
		playInFolderView:function(pathStr){
		
			var filter={filter:[{field:'path',value:''}]};
			filter.filter[0].value=pathStr;
			
			audioPlaylist.clearAndStop();
			
			audioView.searchMedia(filter,function(data){
				//AudioPlayer.playlist=[];
				judgeThumbnail(data.thumbnail,data);
				audioPlaylist.playInFolder(data);
			
			});
			$('#floatingbar').show();	
			AudioPlayer.onplay = scrollbarInit;
		},
		
		/*** 
			Author:Tim
			Desc: To get all filter in the selector attribute.
			@param {object} selector - It may has the some attributes we need.
			@return {object} - the filter object contains a filter array
		*/
		getAllFilterInAttr:function(selector){
		
			var filter={filter:[]};
			
			var temp;
			
			for(var i=0;i<audioFilter.length;i++){
				temp=(getFilterInAttr(selector,audioFilter[i]));
				if(temp.value!=''){
					if(temp.value == 'undefined') // 如歌曲只有artist沒有album則會發生這情況，所以要設成空值，api才抓的到
						temp.value = '';
					filter.filter.push(temp);
				}
			}
			
			return filter;
		},
		
		/*** 
			Author:Tim
			Desc: To get all filter which has the suitable format.If it is suitable originally, we won't process anything.
			@example: var json={artist:'person',album:'funny',genre:'Class',path:'/storage/funny.mp3'};
			@param {object} json - the data will contain the filter info, but it is not suitable format.
			@return {object} - the filter object contains a filter array
		*/
		getFilterFormat:function(json){
		
			var filter={filter:[]};
			
			if(typeof json.filter=='undefined'){
			
				var jsonArr=getAllJsonArr(json);
				
				for(var i=0;i<jsonArr.length-1;i+=2){
					var field={'field':'','value':''};
					field.field=jsonArr[i];
					field.value=jsonArr[i+1];
					filter.filter.push(field);
					if(field.field == 'path' && field.value != '')
						break;
				}
			}else{                                
                if(json.filter[0].field == 'path' && json.filter[0].value != '')
				{ 
					var field={'field':'','value':''};					
						field.field=json.filter[0].field;
						field.value=json.filter[0].value;
						filter.filter.push(field);
				}
				else
					filter=json;
			}
			
			return filter;
		
		},
		
		/*** 
			Author:Tim
			Desc: To search the media data.We will process the json, in order to get the filter which has correct format.
			@param {object} json - the data will contain the filter info.
			@param {object} func - callback function.
			@return {object} - nothing
		*/
		searchMedia:function(json,func){
				
			var filter={filter:[]};
			
			filter=audioView.getFilterFormat(json);
			
			t4u.nimbusApi.fileExplorer.getMediaList(function(data){
				if(t4u.nimbusApi.getErrorCode(data) == 0){
					
					data=getObjEntry(data);
					
					if(typeof data.length=='undefined'){
						func.call(this,data);
					}else{
						for(var i=0;i<data.length;i++){
							func.call(this,data[i]);
						}
					}
				}
			},
			function(data){
			},
			{'mediaType':[{'type':'music'}],'sort':[{'order':'asc','field':'title'}]}
			,filter);
			
		},
		
		/*** 
			Author:Tim
			Desc: An simply API for other class to call playSong function.
			@param {object} selector - It has the some attributes we need.
			@return {object} - nothing
		*/
		playSong:function(selector){
		
			playSong(selector);
			
		},
	
		/*** 
			Author:Tim
			Desc: Convert second to mm:ss
			@param {int} sec - the seconds of song duration.
			@return {string} - the suitable string.
		*/
		convertSecond:function(sec){
	
			if(typeof sec=='undefined')
				return '';
			else if(isNaN(sec))
				return '00:00';
			
			var minute = Math.floor(sec/60);
			var second = sec%60;
			
			if(minute<10) 
				minute='0'+minute;
			
			if(second<10) 
				second='0'+second;
			
			return t4u.format('{0}:{1}',minute,second);
		},
		
		/*** 
			Author:Tim
			Desc: Hide all item and initial some value.
			@param nothing
			@return nothing
		*/
		hideAll:function(){			
			t4u.bgProcess = false;
			$('.detail:visible').hide();
			document.body.scrollTop=0;
			document.documentElement.scrollTop=0;
			topBtn(false);
			this.timer=clearInterval(this.timer);
			this.showAlbumInfo(false);	
			scrollbarInit();			
			$('#div-no-records-found-music').hide();
		},
		
		/*** 
			Author:Tim
			Desc: To setup list view, and show the corresponsive context menu
			@param {string} selector - It is a table will be append data and remove original data.
			@param {object} data - API returns.
			@param {string} isAppend - Whether the data need to append.
			@param {string} mainPanel - If it is undefined, we will use the default value.
			@return nothing
		*/
		setupListView:function(selector,filter,isAppend,mainPanel){
			t4u.abort();
			$(cssSelector.panelImageListView).html('');
			if(typeof filter=='undefined')
				filter={filter:[]};
				
			if(typeof isAppend=='undefined')
				isAppend=false;
				
			audioView.listView.counter=1;
			audioView.filter=filter;
			audioView.listView.isAllData=false;
			audioView.listView.isAppend=false;
			audioView.listView.setup(selector,filter,isAppend);
			
			audioView.hideAll();
			
			if(typeof mainPanel=='undefined')
				$(cssSelector.panelMainListView).show();
			else
				$(mainPanel).show();
				
			showContext([4,5,6]);
		},
		
		/*** 
			Author:Tim
			Desc: To setup image list view, and show the corresponsive context menu
			@param {string} selector - the selector we want to append.
			@param {string} dirStr - the dir.
			@param {object} filter - the filter of search.
			@return nothing.
		*/
		setupImageListView:function(selector,dirStr,filter){
			t4u.abort();
			init();
			if(typeof filter=='undefined')
				filter={filter:[]};
			
			audioView.imageListView.counter=1;
			audioView.dirStr=dirStr;
			audioView.filter=filter;
			audioView.imageListView.isAllData=false;
			audioView.imageListView.isAppend=false;
			audioView.imageListView .setup(selector,dirStr,filter);
			
			audioView.hideAll();
			$(cssSelector.panelMainImageListView).show();
			
			showContext([4,5]);
			
		},
		
		/*** 
			Author:Tim
			Desc: To setup all view which need to watch cookie, and it will to load song info.
			@param {string} cookieName - the cookie name.
			@param {string} selector - It is a table will be append data and remove original data.
			@param {string} mainPanel - If it is undefined, we will use the default value.
			@return nothing
		*/
		setupCookieView:function(cookieName,selector,panelSelector){
			t4u.abort();
			var songs=$.secureEvalJSON(t4u.properties(cookieName));
			
			audioView.hideAll();
			
			if( songs == null)
				return ;
				
			$(selector +' tr').not(":first-child").remove();
			audioView.listView.songNum=1;
			
			audioView.listView.setupUnikeyView(selector,songs.songs);
			$('#divMusicViewerPlList').addClass("playlist");
			scrollbarInit();
			
			if(typeof panelSelector == 'undefined')//recent
				$(cssSelector.panelMainListView).show();
			else//playlist detail
			{
				$(panelSelector).show();
				$(panelSelector + ' table').show();
			}
				
			showContext([4,5,6]);
			
			$('[unikey]:visible').trigger('loadSong');
			setFixedTitle();
		},
		
		setupAllSongView:function(){
			$(cssSelector.btnAllSong).blur();
			mode=modeCategory.allsong;
			audioView.setupListView(cssSelector.panelListView);				
		},
		
		setupRecentView:function(){
			$(cssSelector.btnRecent).blur();
			mode=modeCategory.recent;
			audioView.setupCookieView('plRecent',cssSelector.panelListView);
		},
		
		/*** 
			Author:Tim
			Desc: To setup now playing list view, and it will to load song info.There will set interval to check which song is playing.
			@param nothing
			@return nothing
		*/
		setupNowPlayingView:function(){
			$(cssSelector.btnNowPl).blur();
			t4u.abort();
			audioView.hideAll();

			$(cssSelector.panelListView +' tr').not(":first-child").remove();
			audioView.listView.songNum=1;
			
			var arr=[];
			
			for(var i=0;i<AudioPlayer.playlist.length;i++){
		
				arr.push(AudioPlayer.playlist[i].path);
			}
			
			showContext([7]);
			mode=modeCategory.nowPlayingList;
			
			audioView.listView.setupUnikeyView(cssSelector.panelListView,arr);
			
			$(cssSelector.panelMainListView).show();
			
			$('[unikey]:visible').trigger('loadSong');
			
			showCurrentPlayingSong();
			
			AudioPlayer.onplay = audioView.changeCurrentPlayingSong;
			
		},		
		
		/*** 
			Author:Tim
			Desc: To setup main playlist view.
			@param nothing
			@return nothing
		*/
		setupPlView:function(){
			$(cssSelector.btnPl).blur();
			t4u.abort();
			audioView.hideAll();
			$(cssSelector.panelPlView +' tr').not(":first-child").remove();
			
			
			for(var i=0;i<playlistCookieName.length;i++){
				var list=$.secureEvalJSON(t4u.properties(playlistCookieName[i]));
				if(list==null){
					audioPlaylist.init();
					audioView.setupPlView();
					return;
				}
				$(cssSelector.panelPlView).append(createPlRow(list,i,playlistCookieName[i]));
			}
			
			$(cssSelector.panelMainPlView).show();
			showContext([4,9]);
			mode=modeCategory.playlist;
			audioView.moreSongByThisArtistInPlaylist = false;
		},
		
		/*** 
			Author:Tim
			Desc: To setup details of playlist view.
			@param {string} selector - It is contain the cookie info.
			@return nothing
		*/
		setupPlDetailView:function(selector){
			t4u.abort();
			audioView.hideAll();
			$(cssSelector.panelPlDetailView +' tr').not(":first-child").remove();
			$('#MusicViewrCount-name').attr('cookiename',selector.attr('cookiename'));
			$('#MusicViewrCount-name').text(selector.attr('plname'));
			$('#MusicViewrCount-count').text(selector.attr('pllength'));
			
			if(selector.attr('plLength')!='0'){
				audioView.playlistIndex = parseInt(selector.attr('cookiename').replace('pl', ''), 10) -1;
				audioView.setupCookieView(selector.attr('cookiename'),cssSelector.panelPlDetailView,cssSelector.panelMainPlDetailView);
				showContext([4,6,7]);
			}
			else
				$(cssSelector.panelMainPlDetailView).show();
			audioView.moreSongByThisArtistInPlaylist = false;
		
		},
		
		/*** 
			Author:Tim
			Desc: setupAlbumView.
			@param nothing
			@return nothing
		*/
		setupAlbumView:function(){
			mode=modeCategory.album;
			audioView.setupImageListView(cssSelector.panelImageListView,'album');			
			$(cssSelector.btnAlbum).blur();
		},
		
		/*** 
			Author:Tim
			Desc: setupAritistView.
			@param nothing
			@return nothing
		*/
		setupAritistView:function(){
			mode=modeCategory.artist;
			audioView.setupImageListView(cssSelector.panelImageListView,'artist');				
			$(cssSelector.btnArtist).blur();			
		},
		
		/*** 
			Author:Tim
			Desc: setupGenreView.
			@param nothing
			@return nothing
		*/
		setupGenreView:function(){
			mode=modeCategory.genre;
			audioView.setupImageListView(cssSelector.panelImageListView,'genre');			
			$(cssSelector.btnGenre).blur();			
		},
		setupSearchView:function(keyword){
			mode=modeCategory.search;
			audioView.setupSearchListView(cssSelector.panelListView, keyword);	
		},
		
		/*** 
			Author:Tim
			Desc: To bind the click event for the music top button.
			@param nothing
			@return nothing
		*/
		bindEvent:function(){						
			
			$(document).on('click',cssSelector.btnAllSong,audioView.setupAllSongView);
			$(document).on('click',cssSelector.btnNowPl,audioView.setupNowPlayingView);
			
			$(document).on('click',cssSelector.btnRecent,audioView.setupRecentView);
			$(document).on('click',cssSelector.btnPl,audioView.setupPlView);
			
			$(document).on('click',cssSelector.btnAlbum,audioView.setupAlbumView);
			$(document).on('click',cssSelector.btnArtist,audioView.setupAritistView);
			$(document).on('click',cssSelector.btnGenre,audioView.setupGenreView);
			
			var  btn = $('#getting-Songs').attr('id') ;			
			$(document).on('hover', '#music-tabs >input', function() {
				$(this).css({'background':'#c6c6c6','color':'#5f5e5e'});/*調整hover時bg的顏色*/				
			}).on('mouseout', '#music-tabs >input', function() {
					if(btn!=$(this).attr('id')){
						$(this).css({'background':'','color':'#5f5e5e'});
					}else{
						$(this).css({'background':'#33b5e5','color':'#ffffff'});
					}
			});
			$(document).on('click','#music-tabs >input',function(){
				$('#music-tabs >input').css({'background':'','color':'#5f5e5e'});
				$(this).css({'background':'#33b5e5','color':'#ffffff'});	
				btn=$(this).attr('id');		
				if(btn != 'music_search_result')
					$('#music_search_result').hide();					
				if(typeof controller.audioView != 'undefined')
					controller.audioView.showBackButton(false);
			})						
		},
		
		
		/*** 
			Author:Tim
			Desc: setup the default view(all song) and show it.
			@param nothing
			@return nothing
		*/
		setup:function(){
		
			$(cssSelector.panelbtnMusic).show();
			$(cssSelector.btnAllSong).trigger('click');
			
		},
		
		/*** 
			Author:Steven
			Desc: To setup search view, and show the corresponsive context menu
			@param {string} selector - It is a table will be append data and remove original data.
			@param {string} keyword - Keyword to search
			@return nothing
		*/
		setupSearchListView:function(selector, keyword){
			t4u.abort();
			$(cssSelector.panelImageListView).html('');
			if(typeof filter=='undefined')
				filter={filter:[]};
				
			if(typeof isAppend=='undefined')
				isAppend=false;
				
			audioView.listView.counter=1;
			audioView.filter=filter;
			audioView.listView.isAllData=false;
			audioView.listView.isAppend=false;
			audioView.listView.search(selector, filter, isAppend, keyword);
			
			audioView.hideAll();
			
			if(typeof mainPanel=='undefined')
				$(cssSelector.panelMainListView).show();
			else
				$(mainPanel).show();
				
			audioView.listView.loadingDataFromServer(true);
			$('#div-no-records-found-music').hide();
				
			showContext([4,5,6]);
			
			controller.audioView.showBackButton(false);
		},
		/*** 
			Author:Steven
			Desc: Show album information
			@param {boolean} show - Show the information or not
			@return nothing
		*/
		showAlbumInfo:function(show){
			if(show)
				$('.musicListInfo').show();
			else
				$('.musicListInfo').hide();
		},
		changeCurrentPlayingSong:function(){
			showCurrentPlayingSong();
		}
	};
	audioView.bindEvent();
	window.audioView = audioView;
})(jQuery);