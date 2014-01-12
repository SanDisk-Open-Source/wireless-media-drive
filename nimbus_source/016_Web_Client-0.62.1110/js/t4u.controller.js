;(function($){
	/***
		Desc: Hide all the views
		Author: Steven
		Date: 20120712
	*/
	function hideAll()
	{
		photoView.clear();
		t4u.abort();
		t4u.bgProcess = false;
		$('#divBackButton').hide();
		$('#divBackButton').unbind();
		$('#textfield').val('');
		$(".detail").hide();
		
		var arrBtn=[$('#btnFolderView'),$('#btnPhotosView'),$('#btnVideosView'),$('#btnMusicView'),$('#btnSetting')];
		$.each(arrBtn, function() { $(this).removeClass('menuSelected').attr('style','');});

		$('#music-tabs').hide();
		$('#folder-path').hide();
		
		var selector=$('.cbSelection:checked');
		selector.attr('checked', false);

		topBtn(false);
		hideMusicBar();
		
		showTopBtn(false);
		init();
		//$('#container').css('margin-top', '0');
		$('#bottonList').show();
	}
	
	/***
		Desc: infinite scroll
		Author: Tim
		Date: 20120818
	*/
	function eventInfiniteScroll(){
	
		$('#divFolderView').onScrollBeyond(function() {
			if(mode==modeCategory.folder){
			
				if(folderViewer.isAllData) 
					return;
				
				folderViewer.appendData(true);
				folderViewer.counter+=inifiniteScrollSize;		
				folderViewer.setup(nowPath);
			}
		});
		
		$('#divMusicViewerPic').onScrollBeyond(function() {
			if(mode==modeCategory.album || mode==modeCategory.artist || mode==modeCategory.genre){
			
				if(audioView.filter != null && audioView.dirStr != null ){
					audioView.imageListView.counter+=inifiniteScrollImgSize;
					audioView.imageListView.isAppend=true;
					audioView.imageListView.setup('.musicAlbumsListItems',audioView.dirStr,audioView.filter);
				}
			}
		});
		
		$('#divMusicViewerList table').onScrollBeyond(function() {
			if(mode==modeCategory.allsong){
			
				if(audioView.filter != null){
					audioView.listView.counter+=inifiniteScrollSize;
					audioView.listView.setup('#divMusicViewerList table',audioView.filter,true);
				}
			}
		});
	}
	
	function downloadAlbum(albumId, index, count)
	{
		t4u.nimbusApi.fileExplorer.getMediaList(
			function(data){
				if(t4u.nimbusApi.getErrorCode(data) == 0)
				{
					var entry = getObjEntry(data);
					var items = $.isArray(entry) ? entry : [entry];

					for (var i = 0; i < items.length; i++)
					{
						var item = items[i];
						fileToDownload.push(item.path);
					}

					if (items.length == count)
						downloadAlbum(albumId, index + 100, count);
					else
						makeFrame();
				}
			},
			function(data){
			}, 
			{'mediaType':[{'start':index,'count': count,'type':'images'}],'sort':[{'order':'asc','field':'modified'}]}, 
			{filter:[{'field':'album_id','value': albumId }]}
		);
	}

	var downloadInProgress = false;
	var currentUrlIndex = 0;
	var totalUrlNumbers = 0;
	var fileToDownload = [];

	function OnReadyStateChange()
	{
		console.log('onload');
		var iframe = document.getElementById('iframeid' + (currentUrlIndex - 1));
		var doc = iframe.contentDocument ? iframe.contentDocument : window.frames[iframe.id].document;
		downloadInProgress = false;
		alert(doc.readyState);
		// if (doc.readyState == 'interactive' || doc.readyState == 'complete')
			window.setTimeout(function() {makeFrame();}, 100);
	}
	
	function makeFrame( ) 
	{ 	
		// if (downloadInProgress)
		// 	return;

		downloadInProgress = true;
		if(currentUrlIndex >= fileToDownload.length)
		{
			downloadInProgress = false;
			return;
		}

		var iframeId = 'iframeid' + (currentUrlIndex);
	    $('<iframe id="' + iframeId + '"></iframe>').appendTo('body').hide();
	    $('#' + iframeId).load(function(){
	    	// OnReadyStateChange();
	    	$(this).remove();
	    }).attr( "src", fileToDownload[currentUrlIndex].replace(/^\.?\/\//ig, '') + '?dn' );		

		currentUrlIndex = currentUrlIndex + 1
	    window.setTimeout(function(){makeFrame();}, 100);
	}  
	
	var controller = {
		init: function() {
			//t4u.setting.setup();			
			var _filelists,
				upload = new AjaxUpload('btnUpload',{multiple: true, action: uploadUrl
						, onSubmit: function(file, filelist, extension) {
							_filelists = filelist;
							this._settings.action = '/NimbusAPI/upload?rn&dest=' + encodeURIComponent(folderViewer.currentPath);							
							t4u.elementBlockUI(multiLang.search('Uploading'), function(){
								upload.cancel();
							});
						},
						  onComplete: function(file, response) { 
							t4u.elementUnblockUI();
							if(typeof _filelists !== 'undefined')
							{
								t4u.showMessage({title:multiLang.search('Uploaded successfully'),message:multiLang.search('UploadSuccessMsg').replace('{0}', _filelists.length)});
							}
							else
							{
							t4u.showMessage({title:multiLang.search('Uploaded successfully'),message:file + ':' + multiLang.search('Uploaded successfully')});
							}							
							folderViewer.reload(true); 
							
						},
						onError: function(file) {							
							t4u.elementUnblockUI();
							t4u.showMessage({title:multiLang.search('Upload'),message:file + ':' + multiLang.search('Upload Failed')});
						}
						});  
			$('.musicStop').hide();  
			$('.musicPause').hide();
			$('.musicInfoTouch').click(function() {
				//$('#getting-NowPlayinglists').click();
			});
			
			var isiPadOriPhone = navigator.userAgent.match(/iPad/i) || navigator.userAgent.match(/iPhone/i);
			
			if(!/Chrome/.test(navigator.userAgent) && !isiPadOriPhone) { 
				if(!FlashDetect.versionAtLeast(10)){ // at least v.10 flash to play music
					t4u.showMessage(multiLang.search("Your browser needs Flash plug-in to play music. Click OK to download and install"));
				}  
			}
			this.bind();
		},
		bind: function(){
		
			//Tim
			//eventInfiniteScroll();
		
			//Tim
			$('#btnFolderView').click(function(){
				if(folderViewer.isLoading)
				{
					return;
				}
				hideAll();
				nowPath='/';
				folderViewer.counter=1;
				folderViewer.isAllData=false;
				folderViewer.isAppend=false;
				folderViewer.isLoading = true;
				folderViewer.setup(nowPath);
				$('#divFolderView1').show();
				$('#folder-path').show();
				$(this).addClass('menuSelected');	
				$(this).attr('style','color:white; background:url(../data/images/icons/white_folders.png) 11px 5px no-repeat #33b5e5;');
				mode=modeCategory.folder;	
				hideMusicBar();
				showScrollbar(false);
			});
			
			//Bill
			$('#btnPhotosView').click(function(){
				hideAll();
				albumView.setup();
				$('#divPhotoViewerPhotos').show();
				$(this).attr('style','color:white;background:url(../data/images/icons/white_photos.png) 11px 7px no-repeat #33b5e5;');
				$(this).addClass('menuSelected');					
				mode=modeCategory.photo;
				hideMusicBar();
				showScrollbar(true);
				$('#btnDel').show();
			});
			
			/***
				Author:Bill
				Description:To enter photo lists from Album list by double click				
			*/
			$(document).on('click','[id*="divStackPhotoAlbum"]', function(){
				hideAll();
				$('#btnPhotosView').addClass('menuSelected');	
				$('#btnPhotosView').attr('style','color:white;background:url(../data/images/icons/white_photos.png) 13px 5px no-repeat #33b5e5;');
				var $album = $(this).children('.rotate01');
				photoView.setup($album.attr('albumId'), $album.attr('album'), $album.attr('count'));
				$('#divPhotoViewerAlbum').show();
				$('#btnDownload').show();
				$('#btnDel').show();				
			});
			
			/***
				Author:Bill
				Descrition:Multi select
			*/
			$('.photoAlbumsListItems li a').live("click", function(){
				photoView.selected($(this));
			});
			
			/***
				Description:Go to setting page
			*/
			$('#btnSetting').click(function(){
				hideAll();
				t4u.SS.init();
				$('#page-setting').show();	
				$(this).addClass('menuSelected');				
				$(this).attr('style','color:white; background:url(../data/images/icons/whiteSetting.png?t=5) 13px 6px no-repeat #33b5e5;');				
				mode=modeCategory.setting;
				hideMusicBar();				
				folderViewer.initNimbusCapacity();
				//$('#container').css('margin-top', '-20px');
				$('#bottonList').hide();
				showScrollbar(true);
			});

			/***
				Description:Show help
			*/
			
			$('#btnHelp').tooltipster({ position: 'right',interactive: true,
					offsetX: '-100px',
					content: '<div id="help_tip" ><ul><li><a href="javascript:void(0);"  lang="en" id="btnGetStarted" >'+ multiLang.search('Getting Started') + '</a></li><li><a href="http://kb.sandisk.com/app/answers/detail/a_id/12893/" target="_blank"  lang="en" >' + multiLang.search('Online Support') + '</a></li></ul></div>'
					, trigger:'custom'});
			
			$('#btnHelp').click(function(){
				var btnHelp = $('#btnHelp');				
				
				btnHelp.tooltipster('update', '<div id="help_tip" ><ul><li><a href="javascript:void(0);"  lang="en" id="btnGetStarted" >'+ multiLang.search('Getting Started') + '</a></li><li><a href="http://kb.sandisk.com/app/answers/detail/a_id/12893/" target="_blank"  lang="en" >' + multiLang.search('Online Support') + '</a></li></ul></div>');
				btnHelp.tooltipster('show');
			});

			/***
				Author: Tony
				Description:Go to video page
			*/
			$('#btnVideosView').click(function(){
				hideAll();
				videoView.setup();
				$('#divPhotoViewerVideo').show();
				$(this).addClass('menuSelected');
				$(this).attr('style','color:white; background:url(../data/images/icons/white_videos.png) 12px 8px no-repeat #33b5e5;');
				mode=modeCategory.video;
				hideMusicBar();
				showScrollbar(true);
				$('#btnDownload').show();
				$('#btnDel').show();
			});
			
			/***
				Author: Tim
				Description: Upload
			*/
			$('#btnUpload').click(function(){
				//hideAll();				
				//$(this).addClass('menuSelected');
			});

			/***
				Author: Tim
				Description: Go to music page
			*/
            $('#btnMusicView').click(function(){
				
				audioView.listView.isFirstEnter=true;
				hideAll();
				audioView.setup();
				$(this).addClass('menuSelected');
				$(this).attr('style','color:white; background:url(../data/images/icons/white_music.png) 10px 6px no-repeat #33b5e5;');
				if(typeof controller.audioView != 'undefined')
					controller.audioView.showBackButton(false);
				mode=modeCategory.allsong;
				showScrollbar(true);				
				$('#floatingbar').show();	
				if(!AudioPlayer.isplaying && !AudioPlayer.isPaused)
					$('.noMusicInfo').show();				
				$('#btnDownload').show();
				$('#btnDel').show();
			});	
						
			$(document).on('click','#btnGetStarted', function(){
				hideAll();
				$('#div-gettingStarted').show();
			});
			
			/*** 
				Author:Tim
				Description:Bind the Key Event. 			
			*/			
			$(document).on('keydown','body',keydown);
			$(document).on('keyup','body',keyup);
			$(document).on('click', '.cbSelection', updateMenu);
			
			/***
				Author: Tim
				Description: Bind the buttons of the delete dialog 
			*/
			$(document).on('click','#btnDownload',function(){
				$('#btnDownload').blur();
				var selector=$('.cbSelection:checked');
				if(selector.length == 0)
				{
					var btnDownload = $('#btnDownload');
					btnDownload.tooltipster('update', multiLang.search('No files selected'));
					btnDownload.tooltipster('show');
					setTimeout(function(){
						btnDownload.tooltipster('hide');
					}, 3000);
					return;
				}
				downloadInProgress = false;
				$('iframe').remove();
			
				
				
				var param;
				
				fileToDownload.length = 0;
				currentUrlIndex = 0;
				var errMsg = [];
				for (var i = 0; i < selector.length; i++)
				{
					param = $.parseJSON(selector.eq(i).val());
					if (param.isAlbum)
					{
						downloadAlbum(param.albumId, 0, 100);
						continue;
					}
					if (param.path == '')
						continue;
					if (param.isdir)
					{
						errMsg.push(multiLang.search('Folder downloads are not supported. Please select files instead. '));
						continue;
					}
					fileToDownload.push(param.path);
					//setTimeout(function () { makeFrame(param.path.replace(/^\.?\/\//ig, '') + '?dn' ) }, 1000);
				}
				
				window.setTimeout(function() { makeFrame();}, 100);
				
				if (errMsg.length > 0)
					t4u.showMessage(errMsg.join('<br />\n'));
			});
			
			/***
				Author: Tim
				Description: Bind the buttons of the delete dialog 
			*/
			$(document).on('click','#btnDel',dialogDelete);
			$(document).on('click','#btnDelCancel',function(){t4u.unblockUI()});
			$('#btnDelOK').live('click', function(){
				t4u.blockUIWithTitleAndCallback(multiLang.search('Deleting'), function(){
					t4u.abort();
				});
				eventDel(function(){
					switch(mode){
						case modeCategory.folder:
							folderViewer.setup(nowPath, true);
							break;
						case modeCategory.photo:
							albumView.setup();
							break;
						case modeCategory.photoview:
							photoView.reload();
							break;
						case modeCategory.video:
							videoView.setup();
							break;	
						case modeCategory.nowPlayingList:
							$('#getting-NowPlayinglists').click();
							break;
						case modeCategory.recent:
							$('#getting-Recent').click();
							break;
						case modeCategory.allsong:
							$('#getting-Songs').click();
							break;
						case modeCategory.album:
							$('#music_button_albums').click();
							break;
						case modeCategory.artist:
							$('#music_button_artist').click();
							break;
						case modeCategory.playlist:
							$('#getting-Playlists').click();
							break;
						case modeCategory.genre:
							$('#music_button_genre').click();
							break;
						default:
							break;
					}
					$('#btnDelCancel').trigger('click');
				});
			});
			
			/***
				Author: Steven
				Description: Bind the buttons of the create dialog 
			*/
			$(document).on('click','#btnNew', function(){
				if(folderViewer.currentPath == '/')
				{
					t4u.showMessage(multiLang.search('Cannot create a folder under the root directory'));
					return;
				}
				$('#txtFolderName').val('');
				dialogCreate();
			});
			$(document).on('click','#btnCreateCancel',function(){t4u.unblockUI()});
			$('#btnCreateOK').live('click', function(){			
				if($('#txtFolderName').val() == '')
				{
					t4u.showMessage(multiLang.search('Folder name cannot be empty'));
					return;
				}
				t4u.nimbusApi.fileExplorer.mkdir(
					function(data){
						folderViewer.setup(nowPath, true);
					}, 
					function(data){
						
					}, 
					{path:folderViewer.currentPath + '/' + $('#txtFolderName').val()});
				t4u.unblockUI();
			});
			
			$(document).on('click','.searchBtn',function(){
				
				var $list;
				var filter=$('#textfield').val();
				var mapping = {};
				mapping[modeCategory.folder] = '#divFolderView > table tr:not(:first)';
				mapping[modeCategory.photo] = '.photoAlbumsListItems li';
				mapping[modeCategory.photoview] = '#photoviewer1 li';
				mapping[modeCategory.video] = '#videoListItems li';
				mapping[modeCategory.recent] = '.musicSong';
				mapping[modeCategory.allsong] = '.musicSong';
				mapping[modeCategory.playlist] = '.musicSong';
				mapping[modeCategory.nowPlayingList] = '.musicSong';
				mapping[modeCategory.album] = '.musicAlbumsListItems li';
				mapping[modeCategory.search] = '.musicSong';

				var _mode = mode;
				if($('#divMusicViewerList').is(":visible"))
				{
					_mode = modeCategory.search;
				    mode = modeCategory.search;
				}
				$list = $(mapping[mode]);
				// if (filter == '')
				// {
				// 	$list = $(mapping[mode]);
				// 	$list.show();
				// 	setFixedTitle()
				// 	mode = _mode;
				// 	return;
				// }
			
				switch(mode){
					case modeCategory.folder:
						folderViewer.find(filter);
						break;
					case modeCategory.photo:
					case modeCategory.photoview:
						photoView.find(filter);
						break;
					case modeCategory.video:
						videoView.find(filter);
						break;
					case modeCategory.recent:
					case modeCategory.artist:
					case modeCategory.album:
					case modeCategory.allsong:
					case modeCategory.playlist:
					case modeCategory.genre:
					case modeCategory.nowPlayingList:
					case modeCategory.search:
						$('#music-tabs >input').css({'background':'','color':'#5f5e5e'});
						$('#music_search_result').show();
						$('#music_search_result').css({'background':'#33b5e5','color':'#ffffff'});
						audioView.setupSearchView(filter);
						break;
					default:
						break;
				}
				setFixedTitle()
				mode = _mode;
			});

			/***
				Author: tony
				Description: init Scotti capacity
			*/ 
			folderViewer.initNimbusCapacity();
			t4u.SS.DeviceNameSetting();
			
			$('#textfield').bind('keypress', function(e){
				if (e.keyCode == 13)
					$('.searchBtn').trigger('click');
			});

			/*** 
				Author:Tim
				Description:Set the whole context menu. 			
			*/		
			// bindContext('#divFolderView,#divMusicViewerMusicList table,.musicAlbumsListItems,#divMusicViewerCount table,#divMusicViewerList table');
			bindContext('.thumbnailShowRighClickArrow,.listShowRighClickArrow');
		},

		'hideAll': hideAll
	};
	window.controller = controller;
})(jQuery);
