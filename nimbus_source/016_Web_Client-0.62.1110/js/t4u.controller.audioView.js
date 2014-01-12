;(function($){
	
	/***
		Author: Tim
		Desc: bind mouseover, mouseleave event all of ListView 
		@param nothing
		@return nothing
	*/
	function listViewHover(){
	
		var allAudioListItemStr='#divMusicViewerList table tr:not(":first-child"),#divMusicViewerPlList table tr:not(":first-child"),#divMusicViewerMusicList table tr:not(":first-child")';
	
		$(document).on('mouseenter',allAudioListItemStr ,function(){
			//$(this).attr('enter','enter');
			if($(this).attr('tag')!='clicked'){ 
				$(this).css({'background':'#e9e9e9'});
				$(this).find('.thumbnailShowRighClickArrow').show();
			}
		});
		
		$(document).on('mouseleave',allAudioListItemStr,function(){
			$(this).removeAttr('enter');
			if($(this).attr('tag')!='clicked'){ 
				$(this).css({'background':''});
				$(this).find('.thumbnailShowRighClickArrow').hide();
			}
		});
		
		$(document).on('selected',allAudioListItemStr ,function(){
			$(this).css({'background':'#33b5e5'});
		});
	
	}
	
	/***
		Author: Tim
		Desc: bind click and doubleclick event of ListView 
		@param nothing
		@return nothing
	*/
	function listView(){
	
		var clickItem = '#divMusicViewerList table tr:not(":first-child"),#divMusicViewerPlList table tr:not(":first-child")';
		var clickItemCheckbox = '#divMusicViewerList table tr:not(":first-child") td input[type=checkbox]' 
			+ ',#divMusicViewerPlList table tr:not(":first-child") td input[type=checkbox]'
		var clickPanel='#divMusicViewerList table tr,#divMusicViewerPlList table tr';
		
		$(document).on('click', clickItemCheckbox,function(e){
			eventClick(clickItem,clickPanel,$(this.parentNode.parentNode));
			e.stopPropagation();
		}); 
		
		$(document).on('click', clickItem,function(){
				if(mode == modeCategory.nowPlayingList)
				{
					audioPlaylist.stopAndPlay($(this).index() - 1);
				}
				else
			{
					audioPlaylist.clearAndStop();
				var songsContainer = $(this).parent().find('tr'),
					index = songsContainer.index($(this));
				for(var i=1; i<songsContainer.length; i++)
				{
					audioView.playSong($(songsContainer[i]));				
				}
				audioPlaylist.stopAndPlay(index - 1);
			}
		}); 
	
	}
	
	
	/***
		Author: Tim
		Desc: bind mouseover, mouseleave and doubleclick event of ImageListView 
		@param nothing
		@return nothing
	*/	
	function imageListView(){
	
		var clickItem='.musicAlbumsListItems li a';
		var clickPanel='.musicAlbumsListItems';
	
		$(document).on('mouseenter',clickItem,function(){
			if($(this).parent().find('.pos2').is(':visible'))
			{
			$(this).css({'width':'142px'});
			$(this).css({'height':'142px'});
			$(this).children('img').css({'width':'144px'});
			$(this).children('img').css({'height':'144px'});
			}
			else
			{
				$(this).css({'width':'161px'});
				$(this).css({'height':'161px'});
				$(this).children('img').css({'width':'163px'});
				$(this).children('img').css({'height':'163px'});
			}
			$(this).css({'border':'2px solid #33b5e5'});
			$(this).css({'padding':'2px 0px 0px 2px'});
			$(this).css({'z-index':'5'});
		});
		$(document).on('mouseleave', clickItem,function(){
				if($(this).attr('tag')!='clicked') {
					$(this).css({'border':''});
					$(this).css({'padding':''});
					$(this).css({'width':''});
					$(this).css({'height':''});
				}
		});	
		
		$(document).on('click','.musicAlbumsListItems > li:visible',function(){
			var filter=audioView.getAllFilterInAttr($(this));
			showImageList(filter);	
			if(filter.filter.length > 1)
				audioView.listView.changeTitlePic($(this));			
		});
	}
	
	function showImageList(filter)
	{
		var selector = '#divMusicViewerList table';
		if(typeof filter.filter == 'undefined')
			return;
			
		audioView.imageListView.counter=1;
		audioView.imageListView.isAppend=false;
		audioView.imageListView.isAllData=false;
		audioView.dirStr='album';
		
		audioView.filter=filter;

		if(filter.filter.length>1){			
			audioView.listView.isAllData=false;
			$(selector + ' tr').not(":first-child").remove();//先把之前的資料清除掉
			audioView.listView.setup(selector, filter);
			$('#divMusicViewerPic').hide();
			$('#divMusicViewerList').show();
			
		}
		else{
			audioView.imageListView.setup('.musicAlbumsListItems','album',filter);
		}
		
		showBackButton(true);
	}
	
	/***
		Author: Tim
		Desc: bind the event of the dialog of renaming playlist.
		@param nothing
		@return nothing
	*/	
	function dialogRenamePl(){
		$(document).on('click','#setting-Rename-cancel',function(){
			t4u.unblockUI();
		});
		$(document).on('click','#setting-Rename-ok',function(){
			
			var plName=escape($('#setting-Rename-text').val());
			
			if($.trim(plName)==''){
				t4u.showMessage(multiLang.search(infoPlNameNeed));
				return;
			}
			
			
			for(var i=0;i<playlistCookieName.length;i++){
				if($.secureEvalJSON(t4u.properties(playlistCookieName[i])).name==plName){
					t4u.showMessage(multiLang.search(infoPlNameduplicated));
					return;
				}
			}
			
			var cookie= $('#setting-Rename-text').attr('cookiename');
			var pl=$.secureEvalJSON(t4u.properties(cookie));
			
			pl.name=plName;
			t4u.properties(cookie,JSON.stringify(pl), { expires: cooieExpiredays });
			$('#setting-Rename-cancel').trigger('click');
			audioView.setupPlView();
			reorder_playlist();
			$('#getting-Playlists').trigger('click');
		});
	
	}
	
	/***
		Author: Tim
		Desc: bind the event of the dialog of adding to playlist.
		@param nothing
		@return nothing
	*/	
	function dialogAddToPl(){
		
		
		
		$(document).on('click','#setting-add-to-playlist > .popupMiddle > .popupList02 > li > label',function(){
		
			var popupBlue = {'background':'url(/data/images/icons/grayRadioBtn.png) no-repeat left top'};
			var popupWhite = {'background':'url(/data/images/icons/whiteRadioBtn.png) no-repeat left top'}; 
		
			$('#setting-add-to-playlist').children('.popupMiddle').children('.popupList02').children().children('label').removeClass(cssRadioButtonSelected);	
			$('#setting-add-to-playlist').children('.popupMiddle').children('.popupList02').children().children('label').attr('tag','');		
			$(this).attr('tag','ok');
			$(this).addClass(cssRadioButtonSelected);
		
		});
		
		$(document).on('click','#setting-add-to-playlist-cancel',function(){
			t4u.unblockUI();
		});
		
		$(document).on('click','#setting-add-to-playlist-ok',function(){
		
			
			var plNum=$('[tag="ok"]').index('.fixLabelPos');
			var newName=$('#setting-add-to-playlist').children('.popupMiddle').children('.popupList02').children().children('input[type="text"]').eq(plNum).val();
			newName=escape(newName);
			var oriPl=playlist.get(plNum);
			if(oriPl==null){
				audioPlaylist.init();
				oriPl=playlist.get(plNum);
			}
			
			if(oriPl.songs.length+$('[tag="clicked"]:visible').length > songs_count){
				t4u.showMessage(multiLang.search(infoPlWillFull));
				return;
			}
			
			var source = audioPlaylist.prependList;//$('[tag="clicked"]:visible')
			for(var j=0;j< source.length;j++){
			
				var filter=audioView.getAllFilterInAttr(source[j]),
					path = '';
				
				for(var k=0; k<filter.filter.length; k++)
				{
					if(filter.filter[k].field == 'path')
						path = filter.filter[k].value;
				}

				var plTemplate={oriName:'happy',newName:'fun',songs:[]};
				plTemplate.oriName=oriPl.name;
				plTemplate.newName=newName;
				
				
				
				if(path == '')
				{
					audioView.searchMedia(filter,function(data){
						oriPl=playlist.get(plNum);
						if(oriPl == null)
							return;
						if(oriPl.songs.length >= songs_count){
							t4u.showMessage(multiLang.search(infoPlWillFull));
						}
						else if($.inArray(data.path, oriPl.songs) < 0) {
						plTemplate.songs=oriPl.songs;
						plTemplate.songs.push(data.path);
						playlist.update(plTemplate);
						} 					 				
						else {
							t4u.showMessage(multiLang.search("The song is already in the playlist"));
						}
						
					});
				}
				else
				{
					oriPl=playlist.get(plNum);
						if(oriPl == null)
							return;
						if(oriPl.songs.length >= songs_count){
							t4u.showMessage(multiLang.search(infoPlWillFull));
						}
					else if($.inArray(path, oriPl.songs) < 0) {
						plTemplate.songs=oriPl.songs;
						plTemplate.songs.push(path);
						playlist.update(plTemplate);
						} 					 				
						else {
							t4u.showMessage(multiLang.search("The song is already in the playlist"));
						}
				}
			}
			audioPlaylist.prependList.length = 0;
			
			$('#setting-add-to-playlist-cancel').trigger('click'); 
		});
		
	}
	
	/***
		Author: Tim
		Desc: bind click and doubleclick event of playlist view 
		@param nothing
		@return nothing
	*/
	function playlistControll(){
	
	
		var clickItem='#divMusicViewerMusicList table tr:not(":first-child")';
		var clickPanel='#divMusicViewerMusicList table tr';
		
		$(document).on('click', clickItem,function(){
			eventClick(clickItem,clickPanel,$(this));
			audioView.listView.changeTitlePic($(this));
		}); /***/
		
	
		$(document).on('click', '#divMusicViewerMusicList table tr:not(":first-child")',
			function(){
				showBackButton(true);
				audioView.setupPlDetailView($(this));
			}
		);
	
	}
	
	function showBackButton(show)
	{
		var btnBack = $('#divBackButton');
		var btnTabs = $('#music-tabs');
		if(show)
		{
			btnBack.unbind();
			btnBack.show();
			btnTabs.attr('style', 'margin-left:75px; padding-left:0px');
			btnBack.bind('click', function(){controllerAudio.backButtonClick();});
		}
		else
		{			
			btnBack.hide();
			btnTabs.attr('style', 'margin-left:30px');
			btnBack.unbind();
		}
	}
	
	/*
		Desc:顯示某個Artist or Genre底下的專輯
	*/
	function showAlbumUnderArtistOrGenre()
	{
		audioView.hideAll();
		$('#divMusicViewerPic').show();
		var filter={filter:[]};
		if(mode == modeCategory.genre)
			filter.filter.push(audioView.genreFilter.filter[0]);
		else
			filter.filter.push(audioView.filter.filter[1]);
		showImageList(filter);
	}
	
	var controllerAudio = {
		/***
			Author: Tim
			Desc: bind all events contain the events which set by ourself.
			@param nothing
			@return nothing
		*/
		setup: function(){
			
			listViewHover();
			listView();
			imageListView();
			playlistControll();
			dialogAddToPl();
			dialogRenamePl();
			$(document).on('loadthumbnail','.pos3',getThumbData);
			$(document).on('loadSong','[unikey]:visible',getSongByUnikey);
			
		},
		showBackButton: function(show){
			showBackButton(show);
		},
		backButtonClick: function(){
			switch(mode)
			{
				case modeCategory.artist:
					if(audioView.albumCount == 1 || audioView.filter.filter.length == 1)
						$('#music_button_artist').trigger('click');
					else
						showAlbumUnderArtistOrGenre();
					break;
				case modeCategory.album:
					$('#music_button_albums').trigger('click');
					break;
				case modeCategory.genre:
					if(audioView.albumCount == 1 || audioView.filter.filter.length == 1)
						$('#music_button_genre').trigger('click');
					else
						showAlbumUnderArtistOrGenre();
					break;
				case modeCategory.playlist:
					$('#getting-Playlists').trigger('click');
					break;
				case modeCategory.recent:
					$('#getting-Recent').trigger('click');
					break;
				case modeCategory.allsong:
					$('#getting-Songs').trigger('click');
					break;
				case modeCategory.search:
					$('.searchBtn').trigger('click');
					break;
			}
		}
	};
	
	controllerAudio.setup();
	window.controller.audioView = controllerAudio;
	
})(jQuery);
