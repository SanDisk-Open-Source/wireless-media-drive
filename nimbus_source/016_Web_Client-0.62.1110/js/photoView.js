;(function($){
	/***
		Author : Bill
		Description : Photo Album Search Bind
	*/
	function btnAlbumSearchController(){
		// $('.searchBtn').bind({
				// click:btnAlbumSearchClick
		// });		
	}
	
	/***
		Author : Bill
		Description : Photo Album Search
			  if users do not input any keyword,the system will show all data.
	*/
	function btnAlbumSearchClick(){    		
		
		$('.photoAlbumsListItems li').hide();
		if($('#textfield').val() != '')
			$('.photoAlbumsListItems li[albumName*="'+$('#textfield').val()+'"]').show();
		else
			$('.photoAlbumsListItems li').show();

	}
	
	/***
		Author : Bill
		Description : Photo Album Delete Bind
	*/
	function btnAlbumDelController(){
		// $('#btnDel').bind({
				// click:btnAlbumDelClick		
		// });	
	}
	
	/***
		Author : Bill
		Description : Photo Album Delete
	*/
	function btnAlbumDelClick()
	{
	
	
		var delEvent=function(){
			$('.photoAlbumsListItems li a[class="Selected"]').each(function(){
				var delPath = '/sdcard/' + $(this).attr('id');
				t4u.nimbusApi.fileExplorer.deletePath(function(data){}, function(data){}, {forced:true,path:""+delPath});
			});
			
			albumView.setup();
		}
		dialogDelete(delEvent);
		
	}
	
	
	/***
		Author : Bill
		Description : Photo Album Key Event Bind
	*/
	function keyEventAlbumController(){
		$('body').bind({
				keydown:keydown,
				keyup:keyup
		});	
	}
	
	/***
		Author : Bill
		Description : 當 ctrl or shift 鍵按下時，記錄相關值為 true
	*/
	function keydown(event){
		if(event.keyCode == keyCtrlNum)
			albumView.isCtrl=true;
		else if(event.keyCode == keyShiftNum)
			albumView.isShift=true;
	}
	
	/***
		Author : Bill
		Description : 當 ctrl or shift 鍵放開時，記錄相關值為 false
	*/
	function keyup(event){
		if(event.keyCode == keyCtrlNum)
			albumView.isCtrl=false;
		else if(event.keyCode == keyShiftNum)
			albumView.isShift=false;
	}
	
	/***
		Author : Bill
		Description : init photo album list
	*/
	function photoAlbumSetup(data)
	{		
		var entryObj = getObjEntry(data),
			allData = '';
			
		if(entryObj.length >0)
		{
			if (entryObj.length < albumView.length)
				albumView.allData = true;

			for(var i = 0; i < entryObj.length; i++)
			{
				var index = albumView.index++;
				$('.photoAlbumsListItems').append(createAlbum(entryObj[i], index));
				getPhotoDetailForAlbum(entryObj[i], index);				
			}
		}
		else
		{
			var index = albumView.index++;
			$('.photoAlbumsListItems').append(createAlbum(entryObj, index));
			getPhotoDetailForAlbum(entryObj, index);	
			albumView.allData = true;
			return;
		}
		
		$('.rotate02 img').css({opacity: 0.5}) ;
		$('.rotate03 img').css({opacity: 0.5}) ;	
		
		$('.path').hide();
		
		$('body').unbind('keydown');
		$('body').unbind('keyup');
		keyEventAlbumController();
		
		$('.rotate01').click(function(){
			updateUI(this);
		})
	}
	
	/***
		Author : Bill
		Description : get Entry data
	*/
	function getObjEntry(data)
	{
		if(typeof(data.nimbus.items.entry) == 'undefined')
			return data.nimbus.items;
		else
			return data.nimbus.items.entry;
	}
	
	/***
		Author : Bill
		Description : bind album data for html tag
	*/
	function createAlbum(entryData, nIndex)
	{
		var albumId = entryData.album_id,
			albumName = $.isPlainObject(entryData.album) ? entryData.album['#text'] : entryData.album,
			count = typeof entryData.count === 'undefined' ? '-' : entryData.count,
			result = '';
		if(albumId == undefined)
			return result;
		var checkbox = t4u.ui.createCheckbox({
			name: albumName,
			albumId: albumId,
			path: '',
			thumbnail: '',
			index: nIndex,
			isdir: false,
			isAlbum: true
		});		
		
		result = '<li albumName="' + albumName + '"><div id="divAlbumLoading' + nIndex + '" style="height:95px;padding:85px 0 0 60px;display:none;"><img src="' + virtualPath + '/images/loader.gif" /></div><div id = "divStackPhotoAlbum' + nIndex + '" class="" style="height:150px;display:block">';
		result += '<div id="'+ albumId +'" index="' + (nIndex+1) + '" class="rotate01 showBorder albumPhotoHover"  albumId="'+ albumId + '" album="' + albumName + '" count="'+ count +'" style="position:absolute; top:2px; left:2px; z-index:3;background:' + photoBackColor + '"><span class="addWhiteBorder"><img style="width: 145px; height: 110px;" id="imgAlbum'+ nIndex +'1" src="" /></span></div>';
		result += '<div style="position: relative;height:0px;"><div id="divAlbum' + albumId + '" index="' + (nIndex+1) + '" album="album" ></div></div>';
		result += '</div>';
		result += '<div style="white-space: nowrap;">' + checkbox + showShortAlbumName(fixAlbumName(albumName)) + '<p><span id="spModifiedId'+nIndex+'" style="display:none;" >10, 2012</span>' + multiLang.search('Pictures') + ': ' + count + '</p></div></div>';
		result += '</li>';
		return result;
	}
	
	function fixAlbumName(albumName)
	{
		if(albumName == 'storage')
			albumName = 'Media Drive';
		else if(albumName == 'sdcard')
			albumName = 'Media Drive Card';
		return albumName
	}
	
	function showShortAlbumName(albumName)
	{
		if(albumName == 'storage')
			albumName = 'Media Drive';
		else if(albumName == 'sdcard')
			albumName = 'Media Drive Card';
		var newAlbumName = '<div style="width:133px;" class="shortInfor" title = "' + albumName + '">' + albumName;
		return newAlbumName;
	}
	
	/***
		Author : Bill
		Description : get photo album data 
	*/
	function getPhotoDetailForAlbum(entryData, nIndex)
	{
		var album;
		if(typeof entryData.album == 'object')
			album = entryData.album['#text'];
		else
			album = entryData.album;
		t4u.nimbusApi.fileExplorer.getCategoryThumbnailList(function(data){
			if(t4u.nimbusApi.getErrorCode(data) == 0){	
				if(typeof data.nimbus.items.entry != 'undefined')
				{
					var nCount = 0;
					if(typeof data.nimbus.items.entry.length != 'undefined')
					{
						for(var j=0; j<data.nimbus.items.entry.length; j++)
						{
							var albumId = '#imgAlbum' + nIndex + (j+1);
							$(albumId).attr('src', data.nimbus.items.entry[j].thumbnail);
							fitImage(albumId, 145, 110);
						}
						nCount = data.nimbus.items.entry.length;
					}
					else
					{
						for(var j=0; j<1; j++)
						{
							var albumId = '#imgAlbum' + nIndex + (j+1);
							$(albumId).attr('src', data.nimbus.items.entry.thumbnail);
							fitImage(albumId, 145, 110);
						}
						nCount = 1;
					}
					for(var j=nCount; j<3; j++)
					{
						var albumId = '#imgAlbum' + nIndex + (j+1);
						$(albumId).parent().hide();
					}
				}
				else
				{
					
					var albumId = '#imgAlbum' + nIndex + (1);
					var objImgCover = $(albumId);
					
					objImgCover.hide();
					objImgCover.attr('isAdded', 1);
					for(var j=1; j<3; j++)
					{
						var albumId = '#imgAlbum' + nIndex + (j+1);
						$(albumId).parent().hide();
					}
					t4u.bgProcess = true;
					t4u.nimbusApi.fileExplorer.getMediaList(
						function(data){
							if(t4u.nimbusApi.getErrorCode(data) == 0)
								objImgCover.before(getNoThumbnailBG(album, '144x110'));
						},
						function(data){
						}, 
						{'mediaType':[{'start':1,'count':0,'type':'images'}],'sort':[{'order':'asc','field':'title'}]}, 
						{filter:[{'field':'album','value': album }]});
				}
			}
		},
		function(data){
		},
		{'mediaType':[{'type':'images','start':1,'count':3}]},{filter:[{'field':'album_id','value': entryData.album_id }]});
	}
	
	/***
		Author: Steven
		Description: Show stacked photos
	*/
	function showStackPhoto(nIndex)
	{
		//$('#divAlbumLoading' + nIndex).hide();
		$('#divStackPhotoAlbum' + nIndex).show('slow');
	}	
	
	/***
		Author: Steven
		Description: Reposition Image 
	*/
	function rePositionImage(oImg, albumId)
	{		
		//t4u._debug('albumId:' + albumId);
		var height = 110;
		var width = 145;
		//t4u._debug('height:' + oImg.height);
		//t4u._debug('width:' + oImg.width);
		if(oImg.height/oImg.width > height/width)
		{
			$(albumId).css('height', '' + height);
			var newWidth = Math.ceil((height/oImg.height) * oImg.width);
			$(albumId).css('width', newWidth);			
			//Andrew use center, don't have to do this
			//$(albumId).css('margin-left', Math.floor((width - newWidth)/2));
		}
		else
		{
			$(albumId).css('width', '' + width);
			var newHeight = Math.ceil((width/oImg.width) * oImg.height);				
			$(albumId).css('height', newHeight);
			$(albumId).css('margin-top', Math.ceil((height - newHeight)/2));
		}
		//t4u._debug('newHeight:' + $(albumId).css('height'));
		//t4u._debug('newWidth:' + $(albumId).css('width'));
	}
	
	/***
		Author : Bill
		Description : utc time convert to yyyy/MM/dd HH:mm:ss format
	*/
	function utcToDateStr(utc){
		var date = new Date(utc*1000);
		var result = date.getFullYear() + '/' + (date.getMonth() + 1) + '/' + date.getDate() + ' ' + date.getHours() + ':' + date.getMinutes() + ':' + date.getSeconds();
		return result;
	}
	
	/***
		Author : Bill
		Description : 相簿物件
	*/
	var albumView = {
		isCtrl : false,
		isShift : false,
		lastSelected : 0,
		allData: false,
		isLoading:false,
		index: 1,
		length: inifiniteScrollImgSize,

		initialize: function() {
			this.clear();
			this.allData = false;
			this.index = 1;
			this.isLoading = false;
		},

		setup : function(){
			t4u.abort();
			albumView.loadingDataFromServer(true);
			this.initialize();			
			var albumselector = '.photoAlbumsListItems > li > .cbSelection';
			var $albumSelector = $(albumselector);
			$albumSelector.die('click');
			$albumSelector.live('click', function (){
				eventClick('', albumselector, $(this));
			});

			t4u.scroll.bind($('#content'), function(){
				if (albumView.allData)
				{
					t4u.scroll.unbind();
					return;
				}
				albumView.load();
			});

			this.load();
		},

		load: function() {
			if(!albumView.isLoading) {
			    albumView.isLoading = true;
				t4u.nimbusApi.fileExplorer.getCategoryList(function(data){
						if(t4u.nimbusApi.getErrorCode(data) == 0)
						{
							photoAlbumSetup(data);
							albumView.isLoading = false;
						}
						albumView.loadingDataFromServer(false);
					},
					function(data){
						albumView.loadingDataFromServer(false);
					}, 
					{'category':[{'dir':'album_id'}], 'column': 'album','mediaType':[{'start': albumView.index,'count': albumView.length,'type':'images'}],'sort':[{'order':'asc','field':'album'}]}, '');
			}
		},
		
		clear: function() {
			$('.photoAlbumsListItems').html('');
		},
		loadingDataFromServer: function(loading) {
			if(loading)
			{
				$('#div-dynamicLoading-photo-album').show();
				$('.photoAlbumsListItems').hide();
			}
			else
			{
				$('#div-dynamicLoading-photo-album').hide();
				$('.photoAlbumsListItems').show();
			}
		}
	};
	
	window.albumView = albumView;
	
	/***
		Author : Bill
		Description : Photo Search Bind
	*/
	function btnPhoteSearchController(){
		// $('.searchBtn').bind({
				// click:btnPhotoSearchClick
		// });
	}
	
	/***
		Author : Bill
		Description : Photo Search
	*/
	function btnPhotoSearchClick()
	{
		$('#divPhotoViewerAlbum .photoListItems li').hide();
		if($('#textfield').val() != '')
			$('#divPhotoViewerAlbum .photoListItems  li[photoName*="'+$('#textfield').val()+'"]').show();
		else
			$('#divPhotoViewerAlbum .photoListItems li').show();
	}
	
	/***
		Author : Bill
		Description : Photo Delete Bind
	*/
	function btnPhoteDelController()
	{
		// $('#btnDel').bind({
				// click:btnPhotoDelClick
		// });	
	}
	
	/***
		Author : Bill
		Description : Photo Delete 
	*/
	function btnPhotoDelClick()
	{
		var delEvent=function(){
			for(var i=0;i<$('#photoviewer1 li a[class*="Selected"]').length;i++){
				var delPath =$('#photoviewer1 li a[class*="Selected"]').eq(i).attr('path');
				//alert(delPath);
				t4u.nimbusApi.fileExplorer.deletePath(function(data){}, function(data){}, {forced:false,path:""+delPath});
			}
			
			photoView.setup($('#divPhotoViewerAlbum').attr('albumName'), $('#divPhotoViewerAlbum').attr('count'));
		}
		
		dialogDelete(delEvent);
	}
	
	/***
		Author : Bill
		Description : Photo Key Event Bind
	*/
	function keyEventPhotoController()
	{
		$('body').bind({
				keydown:photoKeydown,
				keyup:photoKeyup
		});	
	}
	
	/***
		Author : Bill
		Description : 當 ctrl or shift 鍵按下時，記錄相關值為 true
	*/
	function photoKeydown(event){
		if(event.keyCode == keyCtrlNum)
			photoView.isCtrl=true;
		else if(event.keyCode == keyShiftNum)
			photoView.isShift=true;
	}
	
	/***
		Author : Bill
		Description : 當 ctrl or shift 鍵放開時，記錄相關值為 false
	*/
	function photoKeyup(event){
		if(event.keyCode == keyCtrlNum)
			photoView.isCtrl=false;
		else if(event.keyCode == keyShiftNum)
			photoView.isShift=false;
	}
	
	/***
		Author : Bill
		Description : init photo list 
	*/
	function handlePhotoList(data, abcount)
	{
		var albumCount = 0;
		var entryObj = getObjEntry(data);
		var AlbumData = '';
		var AlbumID='';

		if(entryObj.length > 0)
		{
			if(typeof entryObj[0].album == 'object')
				AlbumID = entryObj[0].album['#text'];
			else
				AlbumID = entryObj[0].album;
			
			AlbumData = '<h4>' + photoView.albumName + '</h4>';
			
			albumCount = photoView.albumCount;
		}
		else
		{
			photoView.allData = true;
			if (typeof entryObj.album === 'undefined')
				return;
			AlbumData = '<h4>' + photoView.albumName + '</h4>';
			AlbumID = entryObj.album;
			albumCount = 1;
		}
		//AlbumData += '<p>' + utcToDateStr(findLastModifyTime(entryObj)) + '<br />';
		AlbumData += multiLang.search('Pictures')+': ' + albumCount + '</p>';
		
		$('#divPhotoViewerAlbum .photoTitle').html(AlbumData);
		$('#divPhotoViewerAlbum').attr('albumName', AlbumID);
		$('#divPhotoViewerAlbum').attr('count', albumCount);
		
		var photoData = '';
		var photoIds = [];
		if(entryObj.length > 0)
		{
			for(var i = 0; i < entryObj.length; i++)
			{
				var index = photoView.index++;
				var photoId = '#photoitem' + index;
				$('#divPhotoViewerAlbum .photoListItems').append(createPhoto(entryObj[i], index));
				fitImage(photoId, 217, 165);
				//photoData += createPhoto(entryObj[i], i);
			}
		}
		else
		{
			//photoData = createPhoto(entryObj, 0);
			var index = photoView.index++;
			var photoId = '#photoitem' + index;
			$('#divPhotoViewerAlbum .photoListItems').append(createPhoto(entryObj, index));
			fitImage(photoId, 217, 165);
		}
			
        photoEvents();
	}

	var allphotos = [];
	function photoEvents(collection)
	{
		// $('.photoviewer').live("click", function(){
		// 	photoSelected(this);
		// });
		
		
		$('.photoviewer').colorbox({
			rel:'photoview',
				arrowKey: false,
				loop: false,
				slideshow: true,
				slideshowAuto: false,
				opacity: 1,
			fitScreen: true,
			onDatasource: function(settings, index) {
				for (var i = 0; i < allphotos.length; i++)
				{
					allphotos[i]['settings'] = settings;
				}
				if ((allphotos.length - index) == 5 && (index % photoView.length) == (photoView.length - 5))
				{
					t4u.bgProcess = true;
					readData(photoView.albumId, photoView.albumCount, photoView.index, photoView.length);
					t4u.bgProcess = false;
				}

				return allphotos;
			},
			onGetCount: function() {
				return photoView.albumCount;
			},
			onClosed: function(){
				setTimeout(function(){
					init();
				}, 100);
			}
			});
		
		
		//$('#btnDel').unbind('click');
		//$('.searchBtn').unbind('click');
		//btnPhoteSearchController();
		//btnPhoteDelController();
		
		$('body').unbind('keydown');
		$('body').unbind('keyup');
		keyEventPhotoController();
		photoView.isLoading = false;
	}
	
	function appendPhotoToList()
	{

	}
	
	/***
		Autohr : Bill
		Description : 當photo被選到時，要有被圈選的效果
	*/
	function photoSelected(s)
	{
		topBtn(true);
		
		if((photoView.isCtrl == false && photoView.isShift == false) || (photoView.isShift = true && photoView.lastSelected == 0))
		{
			$('.photoviewer').attr('style', '');
			$('.photoviewer').removeClass('Selected');
			//$('.photoviewer').parent().attr('style','border:1px solid black;background-color:' + photoBackColor);
			/**$(s).attr('style', 'border:2px solid #33b5e5;');*/
			//$(s).parent().attr('style','border:2px solid #33b5e5;background-color:' + photoBackColor);
			//alert($(s).parent().parent().html());
			$('.photoListItemHadAblueBorder').hide();
			$(s).parent().parent().find('.photoListItemHadAblueBorder').attr('style','');
			$(s).addClass('Selected');
				
			photoView.lastSelected = Number($(s).attr('index'));
		}
		else if(photoView.isShift == true)
		{
			var startIndex = 0;
			var endIndex = 0;
			if(photoView.lastSelected > Number($(s).attr('index')))
			{
				startIndex = Number($(s).attr('index'));
				endIndex = photoView.lastSelected;
			}
			else
			{
				startIndex = photoView.lastSelected;
				endIndex = Number($(s).attr('index'));
			}
			
			$('.photoviewer').attr('style', '');
			$('.photoviewer').removeClass('Selected');
			
			for(var i = startIndex; i <= endIndex; i++)
			{
				$('.photoviewer [index="' + i + '"]').attr('style', 'border:2px solid #33b5e5;');
				$('.photoviewer [index="' + i + '"]').addClass('Selected');
			}	
		}
		else if(photoView.isCtrl == true)
		{
			if($(s).hasClass('Selected'))
			{
				$(s).attr('style', '');
				$(s).removeClass('Selected');
			}
			else
			{
				$(s).attr('style', 'border:2px solid #33b5e5;');
				$(s).addClass('Selected');
			}
			photoView.lastSelected = Number($(s).attr('index'));
		}
	
		if($('.Selected:visible').length==0)
			topBtn(false);
		
		$('.photoviewer').attr('tag', '');
		$('.Selected:visible').attr('tag','clicked');
		
		//$('.photoviewer').attr('style', '');
		//$('.photoviewer').removeClass('Selected');
		//$(s).attr('style', 'border:2px solid #33b5e5;');
		//$(s).addClass('Selected');
	}
	
	/***
		Autohr : Bill
		Description : find photo album last modify time
	*/
	function findLastModifyTime(entryObj)
	{
		var result = 0;
		if(entryObj.length > 0)
		{
			for (var i = 0; i < entryObj.length; i++)
			{
				if(result < entryObj[i].modified)
					result = entryObj[i].modified;
			}
		}
		else
			result = entryObj.modified;
		return result;
	}
	
	/***
		Autohr : Bill
		Description : create photo data
	*/
	function createPhoto(entryData, nIndex)
	{
		var result = '';
		
		var thumbnail = entryData.thumbnail;
		if(thumbnail == "(NULL)" || typeof thumbnail !== 'string')
			thumbnail = '';
			
		var title = entryData.title;
		if(typeof (title) === "object")
			title = title["#text"];				
		
		var checkbox = t4u.ui.createCheckbox({
			name: title,
			path: entryData.path,
			thumbnail: thumbnail,
			index: nIndex,
			isdir: false
		});
		
		var thumbnailImage = '';
		if (thumbnail == '')
			thumbnailImage = getNoThumbnailBG(entryData.path, '217x165');
		else
			thumbnailImage = '<img id="photoitem' + nIndex + '" class="photoItem" src="' + thumbnail + '"  />';

		result = '<li photoName="' + entryData.title + '"><div class="hoverShowBuleBorder"><div align="center" class="photoListItemsDiv" style="background-color:' 
				+ '#535051'
				+ '"><span class="photoListAddWhiteBorder"><a class="photoviewer" path="' + t4u.encodeURI(entryData.path) 
				+ '" href="' + t4u.encodeURI(entryData.path) + '" rel="photoviewer" index="' + (nIndex+1) + '">'
				+ thumbnailImage + '</a></span></div>' ;
				
		result += '<div style="position:relative; height:0px;"><div class="photoListItemHadAblueBorder" style="display:none;"></div></div></div>';		
		result += '<div class="photoview-checkbox">' + checkbox + '</div>' ;
		
		
		
		result +="</li>";
		return result;
	}
	
	/***
		Autohr : Bill
		Description : photo album 的多選效果
	*/
	function updateUI(s)
	{
		topBtn(true);
		
		s =$(s);
		var div = '#divAlbum' + s.attr('id');
		if((albumView.isCtrl == false && albumView.isShift == false) || (albumView.isShift == true && albumView.lastSelected == 0 ) )
		{
			$('.photoAlbumsListItems li div div div').attr('style', '');
			$('.photoAlbumsListItems li div div div').removeClass('Selected');
			$('.photoAlbumsListItems li div div div[album="album"]').removeClass('albumSelected');
			$(div).addClass('albumSelected');
			$(div).addClass('Selected');
			
			albumView.lastSelected = Number(s.attr('index'));
		}
		else if(albumView.isShift == true )
		{
			var startIndex = 0;
			var endIndex = 0;
			if(albumView.lastSelected > Number(s.attr('index')))
			{
				startIndex = Number(s.attr('index'));
				endIndex = albumView.lastSelected;
			}
			else
			{
				startIndex = albumView.lastSelected;
				endIndex = Number(s.attr('index'));
			}
			
			$('.photoAlbumsListItems li div div div').attr('style', '');
			$('.photoAlbumsListItems li div div div').removeClass('Selected');
			
			for(var i = startIndex; i <= endIndex; i++)
			{
				
				//$('.photoAlbumsListItems li a[index="' + i + '"]').attr('style', 'border:2px solid #33b5e5;');
				$('.photoAlbumsListItems li div div div[index="' + i + '"]').addClass('albumSelected');
				$('.photoAlbumsListItems li div div div[index="' + i + '"]').addClass('Selected');
			}
			
		}
		else if(albumView.isCtrl == true)
		{
			if(s.hasClass('Selected'))
			{
				s.removeClass('Selected');
				$(div).removeClass('albumSelected');
			}
			else
			{
				s.addClass('Selected');
				$(div).addClass('albumSelected');
			}
			
			albumView.lastSelected = Number(s.attr('index'));
		}
		
		if($('.albumSelected:visible').length==0)
			topBtn(false);
		
	}	

	function showItems(collection) {
		if (typeof collection === 'undefined')
			collection = photoView.finder.collection;

		if (collection.size() == 0)
			return;			

		var list = $('#divPhotoViewerAlbum .photoListItems');
		for (var i = 0; i < photoView.length; i++)
		{
			var index = photoView.index++;
			var photoId = '#photoitem' + index;
			var item = collection.get(index - 1);
			if (item == null)
				break;

			list.append(createPhoto(item, index));
			photoView.fit(photoId, 217, 165);
		}

		photoEvents(collection);
		photoView.loadingDataFromServer(false);
	}

	function readData(albumId, albumCount, index, length)
	{
		if (photoView.collection.size() >= photoView.albumCount)
			return;

		t4u.nimbusApi.fileExplorer.getMediaList(function(data){
				// handlePhotoList(data, albumCount);
				if(t4u.nimbusApi.getErrorCode(data) == 0)
				{
					var items = t4u.nimbusApi.getRemoteItems(data);
					photoView.collection.add(items);
					for (var i = 0; i < items.length; i++)
					{
						var item = items[i];
						var href = item.path;
						var title = $.isPlainObject(item.title) ? item.title['#text'] : item.title;
						//空資料
						if(typeof item.path === 'undefined' && items.length == 1)
						{
							photoView.loadingDataFromServer(false);
							return;
						}
						allphotos.push({'title': title, 'link': href, 'settings': null});
					}

					// if (items.length == length)
					// 	readData(albumId, albumCount, index + length, length);
					// else
					{
						var AlbumData = '';

						AlbumData = '<h4>' + fixAlbumName(photoView.albumName) + '</h4>';
						AlbumData += multiLang.search('Pictures')+': ' + albumCount + '</p>';
						
						$('#divPhotoViewerAlbum .photoTitle').html(AlbumData);
						$('#divPhotoViewerAlbum').attr('albumName', albumId);
						$('#divPhotoViewerAlbum').attr('count', albumCount);

						showItems(photoView.collection);
					}
				}
			},
			function(data){
			}, 
			{'mediaType':[{'start':index,'count':length,'type':'images'}],'sort':[{'order':'asc','field':'title'}]}, 
			{filter:[{'field':'album_id','value': albumId }]}
		);
		
	}

	/***
		Author : Bill
		Description : 相簿詳細物件
	*/
	var photoView = {
		isCtrl : false,
		isShift : false,
		lastSelected : 0,
		colorBoxIsLoaded : false,
		albumId: 0,
		albumName: '',
		albumCount: 0,
		index: 1,
		length: inifiniteScrollImgSize,
		allData: false,
		isLoading: false,
		finder: null,
		collection: null,

		initialize: function() {
			photoView.loadColorBox();
			allphotos.length = 0;

			this.index = 1;
			this.allData = false;
			this.clear();
			var $goBack = $('#divBackButton');
			$goBack.show();
			$goBack.bind('click', function() {
				$('#btnPhotosView').trigger('click');
			});

			if (this.collection == null)
				this.collection = new t4u.Collection();
			else
				this.collection.clear();
		},

		find: function(keyword) {

			controller.hideAll();
			$('#btnPhotosView').addClass('menuSelected');	
			$('#btnPhotosView').attr('style','color:white;');
			$('#divPhotoViewerAlbum').show();
			$('#btnPhotosView').attr('style','color:white;background:url(../data/images/icons/white_photos.png) 13px 5px no-repeat #33b5e5;');
			this.initialize();

			photoView.loadingDataFromServer(true);
			
			if (this.finder == null)
			{
				this.finder = new t4u.MediaFinder('images', {
					onDone: function() {
						photoView.isLoading = false;
						photoView.loadingDataFromServer(false);
						//no data found
						if(photoView.finder.collection.get(0) == null)
						{
							$('#div-no-records-found-photo').show();
						}
					},
					onFound: function(data, index, length) {
						if (index <= 1)
							showItems();
					}
				});
			}

			photoView.isLoading = true;
			this.finder.clear();
			this.finder.find(keyword, 1, photoView.length);			
			t4u.scroll.bind($('#content'), function(){
				showItems();
			});
		},

		setup : function(albumId, albumName, albumCount){
			t4u.bgProcess = false;
			this.initialize();
			photoView.loadingDataFromServer(true);
			$('#div-no-records-found-photo').hide();

			mode = modeCategory.photoview;
			this.albumId = albumId;
			this.albumName = albumName;
			this.albumCount = albumCount;
			//cancel bubble of checkbox
			var selection = $('.photoview-checkbox > .cbSelection');
			selection.die('click');
			selection.live('click', function(e){
				eventClick('', '.photoview-checkbox > .cbSelection', $(this));
				e.stopPropagation();
			});
			var menus = [1];
			if (folderViewer.copyPath.path != "")
				menus.push(2);
			showContext(menus);
			//t4u.async = false;
			this.load(albumId, albumCount, photoView.index, photoView.length);
			//t4u.async = true;
			t4u.scroll.bind($('#content'), function(){
				if (photoView.isLoading)
					return;

				if (photoView.allData)
				{
					t4u.scroll.unbind();
					return;
				}
				t4u.bgProcess = true;
				photoView.isLoading = true;
				photoView.load(photoView.albumId, photoView.albumCount, photoView.index, photoView.length);
				// showItems(photoView.collection);
			});
		},

		load: function(albumId, albumCount, start, length)
		{
			readData(albumId, albumCount, start, length);
		},

		reload: function()
		{
			this.clear();
			this.setup(this.albumId, this.albumName, this.albumCount);
		},

		clear : function(){
			$('#divPhotoViewerAlbum .photoTitle').html('');
			$('#divPhotoViewerAlbum .photoListItems').html('');
		},
		loadColorBox : function(){
			if(this.colorBoxIsLoaded)
				return;
			this.colorBoxIsLoaded = true;
			// t4u.loadJs('/data/js/colorbox/colorbox/jquery.colorbox.js');
		},

		fit: fitImage,
		loadingDataFromServer: function(loading) {
			if(loading)
			{
				$('#div-dynamicLoading-photo-list').show();
				$('#photoviewer1').hide();
			}
			else
			{
				$('#div-dynamicLoading-photo-list').hide();
				$('#photoviewer1').show();
			}
		}
	};
		
	window.photoView = photoView;
	
	function fitImage(imageId, maxWidth, maxHeight, callback)
	{
		var imgObj = $(imageId);
		var src = imgObj.attr('src');
		var fileImageWidth = 38,
			itemHeight = 61;
		imgObj.load(function(){

			var changedWidth, changedHeight, changedMargin;
			var props;
			if (this.naturalWidth)
				props = ['naturalWidth', 'naturalHeight'];
			else
				props = ['width', 'height'];
			
			$(this).css('width', this[props[0]]);
			$(this).css('height', this[props[1]]);
			var height = maxHeight;
			var width = maxWidth;
			var oImg = $(this);
			var oldWidth = oImg.width();
			var oldHeight = oImg.height();
			if(oldWidth == null || oldHeight == null)
				return;	
			changedMargin = oImg.css('margin-top');
			if(oldHeight/oldWidth > height/width)
			{
				oImg.css('height', '' + height);
				var newWidth = Math.ceil((height/oldHeight) * oldWidth);
				oImg.css('width', newWidth);	
                                changedWidth = newWidth;
				changedHeight = height;
				if(newWidth<fileImageWidth){				
					oImg.css('margin-right',(20+fileImageWidth-newWidth));
				}
			}
			else
			{
				oImg.css('width', '' + width);
				var newHeight = Math.ceil((width/oldWidth) * oldHeight);				
				oImg.css('height', newHeight);
				if (newHeight < height){
					if(width==fileImageWidth){
						changedMargin = Math.ceil((itemHeight - newHeight)/2);
					}else{
						changedMargin = Math.ceil((height - newHeight)/2);
					}
					oImg.css('margin-top', changedMargin);
				}
				changedWidth = width;
				changedHeight = newHeight;
			}
			if(maxWidth == fileImageWidth)
			{
				oImg.css('padding-left', '8px');		
				oImg.css('margin-right', '8px');	
			}

			if ($.isFunction(callback))
			{
				if (typeof changedMargin === 'string')
					changedMargin = changedMargin.replace('px', '');

				callback.call(this, {
					'size': { 'width': changedWidth, 'height': changedHeight },
					'margin': {'top': changedMargin}
				});
			}

			$(this).unbind();
		});
	}
	
})(jQuery);
