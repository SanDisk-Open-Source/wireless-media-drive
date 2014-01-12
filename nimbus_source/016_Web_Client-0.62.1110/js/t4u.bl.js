
var isShift=false;
var isCtrl=false;
var clickIndex=-1;

/*** 
	Author:Tim
	desc: The function will process the click event of folderView and audioView.
	@param {string} selector - All selectors is binded by the event.
	@param {string} panelSelector - The container(or called parent) contains the all selectors.
	@param {object} thisSelector - the item is clicked.
	@return nothing
 */
function eventClick(selector,panelSelector,thisSelector){

	var clicked = thisSelector.attr('tag')=='clicked';

	var isCheckbox = thisSelector.is(':checkbox');
	var checked = isCheckbox ? 
					thisSelector.is(':checked') : thisSelector.find('input[type=checkbox]').is(':checked');	
	if(isShift && clickIndex!=-1)
	{
		var secondClick= $(panelSelector).index(thisSelector);
		
		var s, e;
		s = Math.min(clickIndex, secondClick);
		e = Math.max(clickIndex, secondClick);
		
		for(var i=s;i<=e;i++){
			$(panelSelector).eq(i).attr('tag', checked ? 'clicked' : '');
			$(panelSelector).eq(i).trigger(checked ? 'selected' : 'mouseleave');
			$checkbox = isCheckbox ? $(panelSelector).eq(i) : $(panelSelector).eq(i).find('input[type=checkbox]');
			if (checked)
				$checkbox.attr('checked', 'checked');
			else
				$checkbox.removeAttr('checked');
		}
		topBtn(checked);
		return;
	}
	else
	{
		var i = $(panelSelector).index(thisSelector);
		$(panelSelector).eq(i).attr('tag', checked ? 'clicked' : '');
		$(panelSelector).eq(i).trigger(checked ? 'selected' : 'mouseleave');
	}
		
	
	thisSelector.attr('tag', clicked ? '' : 'clicked');
	clickIndex=$(panelSelector).index(thisSelector);
	thisSelector.trigger(clicked ? 'mouseleave' : 'mouseenter');
	topBtn($('[tag="clicked"]:visible').length > 0);
		
}

/*** 
	Author:Tim
	Desc: The function will set the buttons of delete and download disable or enable.
	@param {bool} enable - it controll the button  disabled or enabled.
	@return nothing
 */
function topBtn(enable){
	enableBtn($('#btnNew'));
	
	//Derek doesn't want to disable the button	
	enable = true;

	if(enable){
		enableBtn($('#btnDel'));
		enableBtn($('#btnDownload'));
	}else{
		disableBtn($('#btnDel'));
		disableBtn($('#btnDownload'));
	}

}

/*** 
	Author:Tim
	Desc: The function will enabled the buttons of delete and download.
	@param {string} btnSelector - the button need to set enable.
	@return nothing
 */
function enableBtn(btnSelector){

	btnSelector.removeAttr('disabled');
	btnSelector.css('color','#5F5E5E');

}

/*** 
	Author:Tim
	Desc: The function will disabled the buttons of delete and download.
	@param {string} btnSelector - the button need to set disable.
	@return nothing
 */
function disableBtn(btnSelector){

	btnSelector.css('color','#D2D1D1');
	btnSelector.attr('disabled',true);

}


/*** 
	Author:Tim
	Desc: To show the dialog of delete.
	@param nothing
	@return nothing
 */
function dialogDelete(){
	$('#btnDel').blur();
	var selector=$('.cbSelection:checked');
	if(selector.length == 0)
	{
		var btnDelete = $('#btnDel');
		btnDelete.tooltipster('update', multiLang.search('No files selected'));
		btnDelete.tooltipster('show');
		setTimeout(function(){
			btnDelete.tooltipster('hide');
		}, 3000);
		return;
	}	
	t4u.blockUI($('#dialogDel'));
}

/*** 
	Author:Steven
	Desc: To show the dialog of creating folder.
	@param nothing
	@return nothing
 */
function dialogCreate(){
	
	t4u.blockUI($('#dialogCreate'));
	$('#txtFolderName').focus();

}

var albumIndex = 0;
function deletePhotos(albumId, index, count, callback, albumCount)
{
	
	t4u.nimbusApi.fileExplorer.getMediaList(
		function(data){			
			if(t4u.nimbusApi.getErrorCode(data) == 0)
			{
				var entry = getObjEntry(data),
					items = $.isArray(entry) ? entry : [entry],
					nCount = items.length,
					nIndex = 0;

				for (var i = 0; i < items.length; i++)
				{
					var item = items[i];
					t4u.nimbusApi.fileExplorer.deletePath(
						function(data){
							nIndex++;
							if(nIndex == nCount && nCount != count)
							{
								albumIndex++;
								if(albumIndex == albumCount)
									callback();
							}
						}, 
						function(data){}, 
						{forced: false, path:"" + item.path});
				}

				if (items.length == count)
					deletePhotos(albumId, index + 100, count, albumCount);
			}
		},
		function(data){
		}, 
		{'mediaType':[{'start':index,'count': count,'type':'images'}],'sort':[{'order':'asc','field':'modified'}]}, 
		{filter:[{'field':'album_id','value': albumId }]}
	);
}

/*** 
	Author:Steven
	Desc: Check whether it is music releated function
	@param {bool} bIncludePlaylistDetail - include playlist detail or not
	@return nothing
 */
function isMusicView(bIncludePlaylistDetail)
{
	if(mode == modeCategory.recent ||
		mode == modeCategory.artist ||
		mode == modeCategory.album ||
		mode == modeCategory.allsong ||
		mode == (bIncludePlaylistDetail ? modeCategory.playlist : (modeCategory.playlist && !$('#divBackButton').is(':visible'))) ||
		mode == modeCategory.genre ||
		mode == modeCategory.nowPlayingList)
		return true;
	else
		return false;
}

/*** 
	Author:Tim
	Desc: The function will delete file(s) which is selected.
	@param nothing
	@return nothing
 */
function eventDel(callback){
	var path,
		pathArray = [],
		$albumSelector = $('.photoAlbumsListItems li .cbSelection:checked');
	if($albumSelector.length>0){
		var nIndex = 0;
		albumIndex = 0;
		$albumSelector.each(function(){
			var albumId = $(this).parent().parent().find('.rotate01').attr('id');
			deletePhotos(albumId, 1, 100, callback, $albumSelector.length);
		});
		return;
	}
	
	var selector=$('.cbSelection:checked'),
		param,
		itemCount=0;
		
	for(var i=0;i<selector.length;i++){
		
		param = $.parseJSON(selector.eq(i).val());
		
		path = param.path.replace(/^\.?\/\//ig, '/');//$('[tag=clicked]:visible').eq(i).attr('nextPath');
		pathArray.push(path);
		if(typeof path=='undefined'){
			path=$('[tag=clicked]:visible').eq(i).attr('path');
			playlist.removeSong(path);
		}
		
		if(typeof path=='undefined'){
			audioView.deleteAudioInImageList($('[tag=clicked]:visible'));
			return;
		}
		
		t4u.nimbusApi.fileExplorer.deletePath(function(data){
		    // remove from playlist & recent list
			if(t4u.nimbusApi.getErrorCode(data) != 0)
			{
				t4u.unblockUI();
				if(typeof data.nimbus.errmsg === 'string')
					t4u.showMessage(multiLang.search(data.nimbus.errmsg));
				else
					t4u.showMessage(errCode[data.nimbus.errcode]);
				return;
			}
			itemCount++;
			if(isMusicView(false) || mode == modeCategory.folder || mode == modeCategory.search)
			{
				var _cookie_names = ['pl1','pl2','pl3','plRecent'];
				for(var k in _cookie_names) {
					var cookie= _cookie_names[k];
					var pl=$.secureEvalJSON(t4u.properties(cookie));
								if(pl == null)
						continue;
						
					var songs=pl.songs;
					pl.songs = [];//clear

					for(var j = 0; j < songs.length; j++) {
						if(songs[j] != path)
						pl.songs.push(songs[j]);
					}
					t4u.properties(cookie,JSON.stringify(pl), { expires: cooieExpiredays });
				}

				// remove from now playing list
				var a=[];
				var index = -1;
				for(var i=0;i<AudioPlayer.playlist.length;i++){
					if(AudioPlayer.playlist[i].path!=path){
						a.push(AudioPlayer.playlist[i]);
					}
					else
					{
						index = i;
						
					}
				}
				AudioPlayer.playlist=a;
				if(index == AudioPlayer.playindex && index != -1) {
					AudioPlayer.playindex--;
					AudioPlayer.next();
				}
			}
			if(mode == modeCategory.search)
			{
				var filter=$('#textfield').val();
				audioView.setupSearchView(filter);
			}
			//else if(isMusicView(false))
			//	$('input[style*="background"]').trigger('click');
			if(itemCount == selector.length)
			{
				if(mode == modeCategory.playlist && $('#divBackButton').is(':visible'))
				{
					var _cookie_names = ['pl1','pl2','pl3','plRecent'];
					for(var k in _cookie_names) {
						var cookie= _cookie_names[k];
						var pl=$.secureEvalJSON(t4u.properties(cookie));
									if(pl == null)
							continue;
							
						var songs=pl.songs;
						pl.songs = [];//clear

						for(var j = 0; j < songs.length; j++) {
							var bFound = false;
							for(var k =0; k < pathArray.length; k++)
							{
								if(songs[j] == pathArray[k])
								{
									bFound = true;
									break;
								}
							}
							if(!bFound)
								pl.songs.push(songs[j]);
						}
						t4u.properties(cookie,JSON.stringify(pl), { expires: cooieExpiredays });
					}

					// remove from now playing list
					var a=[];
					var index = -1;
					for(var i=0;i<AudioPlayer.playlist.length;i++){
						var bFound = false;
						for(var k =0; k < pathArray.length; k++)
						{
							if(AudioPlayer.playlist[i].path == pathArray[k])
							{
								bFound = true;
								break;
							}
						}
						if(!bFound){
							a.push(AudioPlayer.playlist[i]);
						}
						else
						{
							index = i;
						}
					}
					AudioPlayer.playlist=a;
					if(index == AudioPlayer.playindex && index != -1) {
						AudioPlayer.playindex--;
						AudioPlayer.next();
					}
				}				
				callback();
			}
		},
		function(data){
		},
		{forced: param.isdir, path:"" + path});
	}

}

function inMusicView()
{
	var musicArray = [];
	musicArray.push(modeCategory.recent);
	musicArray.push(modeCategory.artist);
	musicArray.push(modeCategory.album);
	musicArray.push(modeCategory.allsong);
	musicArray.push(modeCategory.playlist);
	musicArray.push(modeCategory.genre);
	musicArray.push(modeCategory.nowPlayingList);
	musicArray.push(modeCategory.search);
	return $.inArray(mode, musicArray) > -1;
}

/*** 
	Author:Tim
	Desc: When the audio is not playing,the music bar need to hide something.The function will hide those items.
	@param nothing
	@return nothing
 */
function hideMusicBar(){

	if(!AudioPlayer.isplaying && !AudioPlayer.isPaused && !inMusicView()){
		//$('.musicBarInfo').children().css('visibility','hidden');
		$('.musicBarInfo').children().not($('.noMusicInfo')).css('display', 'none');
		$('#floatingbar').hide();		
	}
}

/*** 
	Author:Tim
	Desc: When the audio is not playing,the music bar need to hide something.The function will show those items.
	@param nothing
	@return nothing
 */
function showMusicBar(){
	//$('.musicBarInfo').children().css('visibility','');
	$('.musicBarInfo').children().css('display', '');
	$('.noMusicInfo').css('display', 'none');	
	
}


/*** 
	Author:Tim
	Desc: The Global events: keydown.It is the jQuery event
	@param {object} event - the key info.
	@return nothing
 */
function keydown(event){
	if(event.keyCode==keyCtrlNum)
		isCtrl=true;
		
	if(event.keyCode==keyShiftNum)
		isShift=true;
}

/*** 
	Author:Tim
	Desc: The Global events: keyup.It is the jQuery event
	@param {object} event - the key info.
	@return nothing
 */
function keyup(event){
	if(event.keyCode==keyCtrlNum)
		isCtrl=false;
	if(event.keyCode==keyShiftNum)
		isShift=false; 
}

/*** 
	Author:Tim
	Desc: The function will  bind the click event of each item in the context menu.
	@param {string} selectorStr - the selector of context menu.
	@return nothing
 */
function bindContext(selectorStr){

	var items={};
	
	for(var i=0;i<contextMenuStr.length;i++){
	
		var template={name: "",callback: null };
		template.name=multiLang.search(contextMenuStr[i]);
		template.callback=function(key, opt){
			setContextFunc(this, key); 
		};
		items[(i+1)]=template;
	}
	
	$.contextMenu({
		trigger: 'left',
		selector: selectorStr,
		items: items
	});
}

/*** 
	Author:Tim
	Desc: The function will show item(s) in the context menu.
	@param {array} need - the item(s) need to show.
	@return nothing
 */
function showContext(need){

	var el=$('.context-menu-root').children();
	for(var j=0;j<el.length;j++){
		el.eq(j).hide();
	}
	
	for(var j=0;j<need.length;j++){
		el.eq(need[j]-1).show();
	}

}

/*** 
	Author:Tim
	Desc: The function will reduce the unused items.If data.nimbus.items.entry does not exist,we will return data.nimbus.items 
	@param {object} data - the data of api returns.
	@return {object} - the simpler data.
 */
function getObjEntry(data){
		
	if(typeof(data.nimbus.items.entry)=='undefined')
		return data.nimbus.items;
	
		
	return data.nimbus.items.entry;	
}

/*** 
	Author:Tim
	Desc:If the data is a object which contains the node of #text,the function will get its value.
	@param {object} data - the data has the format which is data:{#text:'the value which we need'} .
	@return {object} - the simpler data.
*/
function getObjData(data){
	if(typeof(data) == 'object'){
		return data['#text'];
	}
	else{
		return data;
	}
}

/*** 
	Author:Tim
	Desc:To get the string of json data which is like the function JSON.stringify.
	@param {object} data - An object need to get its string.
	@return {string} - the string of data.
*/
function getAllJsonStr(data){

	var allTag='';
	
	$.each(data, function(key, value) { 
		if(typeof value=='object'){
			allTag+=(key + '= "' + value['#text']+'" '); 
		}else{
			allTag+=(key + '= "' + value+'" '); 
		}
	});

	return allTag;
}

/*** 
	Author:Tim
	Desc:To get the array of json data's value.
	@param {object} data - An object need to get its value.
	@return {array} - the array of data's value.
*/
function getAllJsonValArr(data){

	var allTag=[];
	
	$.each(data, function(key, value) { 
		allTag.push(value); 
	});

	return allTag;
}

/*** 
	Author:Tim
	Desc:To get the array of the whole json data.
	@param {object} data - An object need to get its key and value.
	@return {array} - the array of data's key and value.
*/
function getAllJsonArr(data){

	var allTag=[];
	
	$.each(data, function(key, value) { 
		allTag.push(key); 
		allTag.push(value); 
	});

	return allTag;
}


/*** 
	Author:Tim
	Desc:The function will process the event which needs to process that add the images into this img tag.(asynchronous)
	@param nothing
	@return nothing
*/
function getThumbData(){

	var thisItem=$(this);
	var filter = audioView.getAllFilterInAttr(thisItem);
	var count = filter.filter.length == 2 ? 1 : (thisItem.attr('count') > 3 ? 3 : thisItem.attr('count'));
	//t4u._debug(filter.filter[0].field + ':' + filter.filter[0].value + ':' + count);
	t4u.nimbusApi.fileExplorer.getCategoryThumbnailList(function(data){
		if(t4u.nimbusApi.getErrorCode(data) == 0){
			setImgThumb(data, thisItem);
		}
			
	},
	function(data){
	}
	,{'mediaType':[{'type':'music', 'count':count}],'category':'album'}
	,filter);
}

/*** 
	Author:Tim
	Desc:The function will process the event which needs to process that add the images into this img tag.(asynchronous)
	@param {object} data - the api returns.
	@param {object} thisItem - the selector which is processing now.
	@return nothing
*/
function setImgThumb(data, thisItem){	
	if(typeof data.nimbus.items.entry != 'undefined')
	{
		if(typeof data.nimbus.items.entry.length != 'undefined')
		{
			for(var j=0; j<data.nimbus.items.entry.length; j++)
			{
				//t4u._debug(thisItem.attr('artist') + ':' + j + data.nimbus.items.entry[j].thumbnail);
				var objImg = (j == 0 ? thisItem : thisItem.parent().parent().children('.pos' + (3-j)));
				objImg.attr('src', data.nimbus.items.entry[j].thumbnail);
				objImg.show();
			}
		}
		else
		{
			for(var j=0; j<1; j++)
			{
				//t4u._debug(thisItem.attr('artist') + ':' + j + data.nimbus.items.entry.thumbnail);
				var objImg = thisItem;
				objImg.attr('src', data.nimbus.items.entry.thumbnail);
				objImg.css({'width':'163px'});
				objImg.css({'height':'163px'});
				objImg.show();
			}
		}
	}
	//沒有圖，要把預設圖變成165px*165px
	else
	{		
		var objImg = thisItem;
		objImg.parent().parent().find('img').css({'width':'163px'});
		objImg.parent().parent().find('img').css({'height':'163px'});
	}	
}

/*** 
	Author:Tim
	Desc:The function need to judge whether the thumbnail is too small to put on the web or whether the thumbnail is dead.(asynchronous)
	@param {string} thumbnailPathStr - the path of thumbnail.
	@param {object} thisItem - the selector which is processing now.
	@return nothing
*/
function judgeThumbnail(thumbnailPathStr,thisItem){
	if(typeof thumbnailPathStr == 'undefined' || thumbnailPathStr == 'undefined')
	{
		var unique_key='';
		if((mode == 'artist' || mode == 'album' || mode == 'genre') && typeof thisItem.context != 'undefined') {
			unique_key = thisItem.parent().parent().parent().attr(mode);
			thisItem.after(getNoThumbnailBG(typeof unique_key == 'undefined'?'none':unique_key, '144x144'));
			thisItem.remove();
		}
	}
	else
	{
		thisItem.albumphoto=thumbnailPathStr;
		$('#albumphoto').parent().removeClass('thumbnailGenerator_front01');
	}
}

/*** 
	Author:Tim
	Desc:The function will return the first selector which does not contain the thumbnail we need. (Recursive,at most stack three times)
	@param {object} selector - the selector which is processing now.
	@return {object} - the first selector which does not contain the thumbnail we need. 
*/
function judueThumbnailContainer(selector){
	
	if( (typeof selector.attr('src') != 'undefined') && selector.attr('src')!=loadImg && selector.attr('src')!=defaultImg  ){
		
		switch(selector.attr('class')){
			case 'pos3':
				selector = selector.parent().parent().children('.pos2');
				selector=judueThumbnailContainer(selector);
				break;
			case 'pos2':
				selector = selector.parent().children('.pos1');
				selector=judueThumbnailContainer(selector);
				break;
			case 'pos1':
				selector = false;
				break;
		};
	}
	
	return selector

}

/*** 
	Author:Tim
	Desc:The function will process the event that the selector only contains the unikey and we will use the unikey to add the all data .(asynchronous)
	@param nothing
	@return nothing
*/
function getSongByUnikey(){

	var item=$(this);
	var filter=audioView.getAllFilterInAttr(item);
	var func=function(data){
	
		if(typeof data.title == 'undefined' && typeof data.artist == 'undefined' && typeof data.duration == 'undefined')
		{
			//remove from cookie and playlist
			item.remove();
			playlist.removeSong(filter.filter[0].value);						
			return;
		}
			
		var arr=getAllJsonArr(data);
		for(var i=0;i<arr.length-1;i+=2){
			item.attr(arr[i],getObjData(arr[i+1]));
		}
		
		var time=audioView.convertSecond(data.duration);
		item.children().eq(1).children('b').text(item.index());
		item.children().eq(2).text(typeof data.title == 'string'?data.title:'');
		item.children().eq(3).text(typeof data.artist == 'string'?data.artist:'');
		item.children().eq(4).text(time);
		setFixedTitle();
	};
	audioView.searchMedia(filter,func);
}

/*** 
	Author:Tim
	Desc: To Process something according to the menuItemStr.
	@param {string} menuItemStr - From 1 to 8,the names of menu item is in the define 
	@return nothing
*/
function setContextFunc(source, menuItemStr){

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
		panelbtnMusic:'#music-tabs'
	
	};
	
	// var clicked=$('[tag="clicked"]:visible');
	
	// check if selector has musicAlbumsListItems class beacause of album/artist adding playist problem
	var clicked = source.parent().parent().hasClass('musicAlbumsListItems')?source.parent():source.parent().parent();
		
	menuItemStr=parseInt(menuItemStr);
	
	if(clicked.length==0 && menuItemStr !=2 &&  menuItemStr !=4 ){
		t4u.showMessage( multiLang.search( infoContextNoSelect ) );
		return;
	}

	//alert(menuItemStr);
	switch(menuItemStr){
		case 1: //copy selected
			//清空copy的東西
			folderViewer.copyPath.path = '';
		
			folderViewer.copySelected = [];
			var selected=$('.cbSelection:checked');
							
			for(var i=0; i<selected.length; i++)
			{
				source = selected.eq(i);
				var param = source.parent().parent().hasClass('musicAlbumsListItems')?source.parent():source.parent().parent();
					obj = {path: param.attr('nextPath'), isDir: param.attr('isdir') == 'true', size: param.attr('bytes')};
				folderViewer.copySelected.push(obj);
			}
					
			//clear selected
			selected.trigger('click');
			
			updateMenu();
			
			break;
		case 2: //copy
			//clear copy selected
			folderViewer.copySelected = [];
			
			folderViewer.copyPath.path = clicked.attr('nextPath');
			folderViewer.copyPath.size = clicked.attr('bytes');
			folderViewer.copyPath.isDir = clicked.attr('isdir') == 'true';
			updateMenu();
			break;
		case 3: //paste
			var srcObj = folderViewer.copyPath,
				basedir = function(str){
					var pos = str.length;
					if (str.charAt(str.length - 1) == '/')
						pos--;

					return str.substr(0, str.lastIndexOf('/', pos) + 1);
				},
				basename = function(str) {
					if (str == '/' || str.lastIndexOf('/') == -1)
						return;
					else
						return str.substr(str.lastIndexOf('/') + 1);
				};
			if(srcObj.path != ''){
				
				var isDir = srcObj.isDir;
				var isDirDst = clicked.attr('isdir') == 'true';
				var srcPath = basedir(srcObj.path);
				var dstPath = clicked.attr('nextPath');
				if (!isDirDst)
					dstPath = basedir(dstPath);
				dstPath = dstPath.charAt(dstPath.length - 1) == '/' ? dstPath : dstPath + '/';
				var dstName = basename(srcObj.path);
				if (srcPath == dstPath || dstPath.indexOf(srcObj.path) != -1)
				{
					t4u.showMessage(multiLang.search('Target and souce folders cannot be the same'));
					break;
				}
				//如果目的是fat32，而且來源是檔案，而且大小超過4GB，要擋掉
				if(folderViewer.fileSystemIsFat32(dstPath) && !isDir && srcObj.size > 4*1024*1024*1024)
				{
					t4u.showMessage(multiLang.search('The file size is larger than 4 GB. Please reduce the file size or reformat to FAT32'));
					break;
				}

				var dataJson={'src': srcObj.path,'dest': dstPath + (isDir ? '' : dstName), 'force': true};
				t4u.blockUIWithTitleAndCallback(multiLang.search('Copying'), function(){
					t4u.abort();
				});
				default_api_timeout = timeout.long; // more execute time 
				t4u.nimbusApi.fileExplorer.copy(function(data){
					t4u.unblockUI();
					if(t4u.nimbusApi.getErrorCode(data) == 0)
					{
						folderViewer.reload();
						default_api_timeout = timeout.short; // set to default value
					}
				},
				function(data){
					t4u.unblockUI();
				},
				dataJson);
			}
			else// copy selected
			{
				var selectedLength = folderViewer.copySelected.length,
					selectIndex = 0;
				for(var i=0; i<folderViewer.copySelected.length; i++)
				{
					var srcObj = folderViewer.copySelected[i];
					var isDir = srcObj.isDir;
					var isDirDst = clicked.attr('isdir') == 'true';
					var srcPath = basedir(srcObj.path);
					var dstPath = clicked.attr('nextPath');
					if (!isDirDst)
						dstPath = basedir(dstPath);
					dstPath = dstPath.charAt(dstPath.length - 1) == '/' ? dstPath : dstPath + '/';
					var dstName = basename(srcObj.path);
					if (srcPath == dstPath || dstPath.indexOf(srcObj.path) != -1)
					{
						t4u.showMessage(multiLang.search('Target and souce folders cannot be the same'));
						break;
					}
					//如果目的是fat32，而且來源是檔案，而且大小超過4GB，要擋掉
					if(folderViewer.fileSystemIsFat32(dstPath) && !isDir && srcObj.size > 4*1024*1024*1024)
					{
						t4u.showMessage(multiLang.search('The file size is larger than 4 GB. Please reduce the file size or reformat to FAT32'));
			break;
					}

					var dataJson={'src': srcObj.path,'dest': dstPath + (isDir ? '' : dstName), 'force': true};
					t4u.blockUIWithTitleAndCallback(multiLang.search('Copying'), function(){
						t4u.abort();
					});
					default_api_timeout = timeout.long; // more execute time 
					t4u.nimbusApi.fileExplorer.copy(function(data){						
						if(t4u.nimbusApi.getErrorCode(data) == 0)
						{
							selectIndex++;
							if(selectIndex == selectedLength)
							{
								t4u.unblockUI();
								folderViewer.reload();
								default_api_timeout = timeout.short; // set to default value
							}							
						}
					},
					function(data){
						t4u.unblockUI();
					},
					dataJson);
				}
			}
			break;
		case 4: //play
			audioPlaylist.clearAndStop();
			if(typeof clicked.attr('cookiename') !='undefined'){
				var index=clicked.index()-1;
				audioPlaylist.playAllPlayListSongs(index);
				return;
			}
			
			if(typeof clicked.attr('path') =='undefined'){
				audioView.getAllSongsBySearch(clicked);
				return;
			}
			
			clicked.trigger('click');
			//playSong();
			
			break;
		case 5: //add to playlist
			if($('.cbSelection:checked').length > 1) {
			    t4u.showMessage(multiLang.search("Add one song at-a-time to the playlist"));
			    break;
			}
			var txt = $('#setting-add-to-playlist').children('.popupMiddle').children('ul').children().children('input[type!="checkbox"]');
			for(var i=0;i<playlistCookieName.length;i++){
				var pl=playlist.get(i);
				if(pl == null)
					continue;
				txt.eq(i).val(unescape(pl.name));
			}
			audioPlaylist.prependList.push(clicked);
			t4u.blockUI($('#setting-add-to-playlist'));
			$('#setting-add-to-playlist1:visible').trigger('click'); //To prevent that the user does not select. 
			break;
		case 6:
			var filter={filter:[{field:'artist',value:''}]};
			filter.filter[0].value=clicked.attr('artist');
			//audioView.setupListView(cssSelector.panelListView,filter,false);
			audioView.setupImageListView(cssSelector.panelImageListView,'album',filter);
			audioView.moreSongByThisArtistInPlaylist = true;
			break;
		case 7: //remove from now Playing List			
			var tagClick=clicked;
			var cookie= $('#MusicViewrCount-name:visible').attr('cookiename');
			if(typeof cookie=='undefined'){
				var a=[];
				for(var i=0;i<AudioPlayer.playlist.length;i++){
					if(AudioPlayer.playlist[i].path!=tagClick.attr('path')){
					a.push(AudioPlayer.playlist[i]);
					}					
					}
					
				AudioPlayer.playlist=a;
				
				
				if($('#divMusicViewerList-List tr.musicSong.nowPlaying').attr('path') == tagClick.attr('path')) {
					AudioPlayer.playindex--;
					AudioPlayer.next();
				}
				
				tagClick.remove();
				
				return;
			}
			
			tagClick.remove();
			var pl=$.secureEvalJSON(t4u.properties(cookie));
			pl.songs=[];
			//var dataJson={oriName:pl.name,newName:pl.name,songs:[]};
			
			for(var i=0;i<$('table tr:visible').not(':first-child').length;i++){
				var temp=escape($('table tr:visible').not(':first-child').eq(i).attr('path'));
				
				pl.songs.push(temp);
			}
			//playlist.update(dataJson);
			t4u.properties(cookie,JSON.stringify(pl), { expires: cooieExpiredays });
			$('#MusicViewrCount-count').text($('table tr:visible').not(':first-child').length);
			$(cssSelector.btnPl).trigger('click');
			$(cssSelector.panelPlView).children().children('[cookiename="'+cookie+'"]').trigger('click');
			break;
		case 8:
							
			clicked.trigger('click');
				
			break;
		case 9:
			$('#setting-Rename-text').val(clicked.attr('plname'));
			$('#setting-Rename-text').attr('cookiename', clicked.attr('cookiename'));
			t4u.blockUI($('#setting-Rename'));
			break;
		case 10://add to Now Playing List
			if(mode == modeCategory.playlist)
			{
				if($('#divBackButton').is(':visible'))//明細
				{
					var playlistIndex=audioView.playlistIndex,
						songIndex = clicked.index()-1,
						pl= playlist.get(playlistIndex),
						uniKey = playlist.getSong(playlistIndex, songIndex),
						filter=unescape(uniKey);
					t4u.bgProcess = true;
					t4u.nimbusApi.fileExplorer.getMediaList(function(dataJson){
							if(t4u.nimbusApi.getErrorCode(dataJson) == 0)
							{
								if(typeof dataJson.nimbus!='undefined'){
									dataJson=getObjEntry(dataJson);
								}
							
								if(typeof dataJson.path != null)
								{
									var musicFormat=dataJson.path.split('.');
									musicFormat=""+musicFormat[musicFormat.length-1];
									
									var result={albumname:dataJson.album,songname:dataJson.title,albumphoto:dataJson.albumphoto,artist:dataJson.artist};						
									judgeThumbnail(dataJson.thumbnail,result);							
									result[musicFormat]=dataJson.path;							
									result['path']=dataJson.path;
									AudioPlayer.playlist.push(result );
								}
							}
							t4u.bgProcess = false;
						},
						function(data){
						},
						{'mediaType':[{'type':'music'}],'sort':[{'order':'asc','field':'title'}]},
						{filter:[{'field':'path','value':filter}]}
					);				
				}
				else
				{
					var index=clicked.index()-1;
					var pl= playlist.get(index);
					for(var i=0;i<pl.songs.length;i++){
						var uniKey = playlist.getSong(index, i);
						var filter=unescape(uniKey);
						t4u.bgProcess = true;
						t4u.nimbusApi.fileExplorer.getMediaList(function(dataJson){
								if(t4u.nimbusApi.getErrorCode(dataJson) == 0)
								{
									if(typeof dataJson.nimbus!='undefined'){
										dataJson=getObjEntry(dataJson);
									}
								
									if(typeof dataJson.path != null)
									{
										var musicFormat=dataJson.path.split('.');
										musicFormat=""+musicFormat[musicFormat.length-1];
										
										var result={albumname:dataJson.album,songname:dataJson.title,albumphoto:dataJson.albumphoto,artist:dataJson.artist};						
										judgeThumbnail(dataJson.thumbnail,result);							
										result[musicFormat]=dataJson.path;							
										result['path']=dataJson.path;
										AudioPlayer.playlist.push(result );
									}
								}
								t4u.bgProcess = false;
							},
							function(data){
							},
							{'mediaType':[{'type':'music'}],'sort':[{'order':'asc','field':'title'}]},
							{filter:[{'field':'path','value':filter}]}
						);				
					}
				}
				
			}
			else
			{
				var source = [];
				source.push(clicked);
				for(var j=0;j< source.length;j++){
					var filter=audioView.getAllFilterInAttr(source[j]);
					audioView.searchMedia(filter,function(data){
						var dataJson={albumname:"Kalimba",songname:'Kalimba',albumphoto:null,artist:''};
						dataJson.albumname=data['album'];
						dataJson.songname=data['title'];
						dataJson.path=data['path'];

						var thumbnail=data['thumbnail'];
						judgeThumbnail(thumbnail,dataJson);
						dataJson.artist=data['artist'];
						dataJson[data['path'].split('.')[data['path'].split('.').length-1]]=t4u.encodeURI(data['path']);
						dataJson['path']=data['path'];
						AudioPlayer.playlist.push(dataJson);
						
					});
				}
				//AudioPlayer.playlist.push(audioView.getSongJson(clicked));
			}
			break;
	}
	
	return;

}

/*** 
	Author:Tim
	Desc:  The function will return whether the machine is mobile. (It can not judge the tablet,so we need to change.)
	@param nothing
	@return {bool} - whether the machine is mobile? 
*/
function isMobile(){
	var mobiles = new Array
	(
		"midp", "j2me", "avant", "docomo", "novarra", "palmos", "palmsource",
		"240x320", "opwv", "chtml", "pda", "windows ce", "mmp/",
		"blackberry", "mib/", "symbian", "wireless", "nokia", "hand", "mobi",
		"phone", "cdm", "up.b", "audio", "sie-", "sec-", "samsung", "htc",
		"mot-", "mitsu", "sagem", "sony", "alcatel", "lg", "eric", "vx",
		"NEC", "philips", "mmm", "xx", "panasonic", "sharp", "wap", "sch",
		"rover", "pocket", "benq", "java", "pt", "pg", "vox", "amoi",
		"bird", "compal", "kg", "voda", "sany", "kdd", "dbt", "sendo",
		"sgh", "gradi", "jb", "dddi", "moto", "iphone", "android",
		"iPod", "incognito", "webmate", "dream", "cupcake", "webos",
		"s8000", "bada", "googlebot-mobile"
	)
	var ua = navigator.userAgent.toLowerCase();		
	for (var i = 0; i < mobiles.length; i++) {
		if (ua.indexOf(mobiles[i]) > 0) {
			return true;
		}
	}
	return false;
}

function showTopBtn(bShow)
{
	if(bShow)
	{
		$('#btnUpload').show();
		$('#btnNew').show();
		$('#btnDownload').show();
		$('#btnDel').show();
	}
	else
	{
		$('#btnUpload').hide();
		$('#btnNew').hide();
		$('#btnDownload').hide();
		$('#btnDel').hide();
	}
}

//檢查是否有項目被選擇
function updateMenu()
{
	if(mode != modeCategory.folder)
		return;
	var menu = [];
	if($('.cbSelection:checked').length > 0)
	{
		menu.push(1);
	}	
	else
	{
		menu.push(2);
	}
	if (folderViewer.copyPath.path != "" || folderViewer.copySelected.length > 0)
	{
		menu.push(3);
	}
	showContext(menu)
}