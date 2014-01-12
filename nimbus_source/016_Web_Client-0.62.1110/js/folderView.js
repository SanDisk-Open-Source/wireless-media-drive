;(function($){
	
	/*** 
	*	Author:Tim/Tony
	*	Desc:the function is the ajax callback function which is used by the API of getDir.
	*	@param {object} data - the data of API returns.
	*	@return nothing
	*/
	function getData(data){
		
		//if errcode not 0,we won't process
		if(t4u.nimbusApi.getErrorCode(data) != 0) return;
	
		var entryObj= getObjEntry(data);	
		
		if(folderViewer.isAllData) return;

		if(entryObj.length > 0){
		
			if(entryObj.length <inifiniteScrollSize) {
				folderViewer.isAllData=true;
			}								
				
			for(var i=0;i<entryObj.length;i++){
				$('#divFolderView table').append(createRow(folderViewer.index++, getFolderData(entryObj[i])));
				photoView.fit('#file' + folderViewer.index);
			}
		}
		else
		{
			if(typeof entryObj.path=='undefined') {
				folderViewer.isAllData=true;
				return;   //if the entryObj is null,we won't push any data.
			}
			
			if(inifiniteScrollSize>1 ) {
				folderViewer.isAllData=true;
			}
			
			// folderViewer.counter++;
			$('#divFolderView table').append(createRow(folderViewer.index++, getFolderData(entryObj)));
			photoView.fit('#file' + folderViewer.index);
		}
		
		//folderViewer.sort('name',true);
		
		// if(typeof $.colorbox == 'undefined')
		// t4u.loadJs('/data/js/colorbox/colorbox/jquery.colorbox.js');
		
		$('.photoviewer').colorbox({
			rel:'folderview',
			arrowKey: false,
			loop: false,
			slideshow: true,
			slideshowAuto: false,
			opacity: 1,
			fitScreen: true
		});
		
		//folderViewer.counter+=inifiniteScrollSize;
		setTableWidthByTable($('#divFolderView .dropBoxDivContainer'), $('#dropBoxDivTitle'));
		setTableWidthByTable($('#divFolderView .dropBoxDivContainer'), $('#divFolderView .dropBoxDivContainer'));//Andrew設定內容也要有width
		folderViewer.isLoading = false;
	}
	
	/*** 
	*	Author:Bill
	*	Desc:the function will generate the friendlier JSON which contain name,date,format,is_dir,size,path.
	*	@param {object} item - the data of API returns.
	*	@return {object} - the simply data.
	*/
	function getFolderData(item)
	{
		var temp={name:null,modified:null,modifiedUTC:null,format:null,is_dir:null,bytes:null,size:null,path:null,nextPath:null};
		if(typeof item.path === 'undefined')
			return null;
		temp.name="" + item.path;			
		temp.modified=utcToDateStr(item.modified);
		temp.modifiedUTC=item.modified;
		temp.format=fileType(item.path,item.is_dir);
		temp.is_dir=item.is_dir;
		temp.bytes=item.bytes['#text'];
		temp.thumbnail=item.thumbnail;
		if(!temp.is_dir)
			temp.size=convertByte(item.bytes);
		else
			temp.size='&nbsp;';
		temp.path=nowPath;
		temp.nextPath = (("" + item.path).charAt(0) == '/' ? ("" + item.path) : (nowPath == '/' ? '' : nowPath) + (temp.name.charAt(0) == '/' ? '' : '/') + temp.name);
		return temp;
	}
	
	/*** 
	*	Author:Tim
	*	Desc:the function will return file(s) type.
	*	@param {string} path - the path of this item.
	*	@param {bool} is_dir - Whether the item is folder.
	*	@return {string} - Five type: folder, photo, video, audio and file.
	*/
	function fileType(path,is_dir){
		
		if(is_dir) return 'folder';
		
		var fileFormat=path.split('.');
		fileFormat=fileFormat[fileFormat.length-1].toLowerCase();
		
		var audioType= ["mp3","wav","m4a","flac","wma","mpga","ape","wave","aac","ogg","oga","aif","ief","ac3"];
		var videoType=["avi","mp4","mpg","mpeg","m4v","mp4v","divx","xvid","mkv","3gp","3g2","mov","wmv","skm","asf","flv","ogv"];		
		//var imgType=["jpg","jpeg","tif","tiff","png","bmp","gif","pcx"];		
		var imgType=["jpg","jpeg","png","bmp","gif","pcx"];		
		
		for(var i=0;i<imgType.length;i++)
			if(fileFormat==imgType[i]) return 'photo';
		
		for(var i=0;i<videoType.length;i++)
			if(fileFormat==videoType[i]) return 'video';
			
		for(var i=0;i<audioType.length;i++)
			if(fileFormat==audioType[i]) return 'audio';
		
		return 'file'; //other	
	}
	
	
	/*** 
	*	Author:Bill
	*	Desc:Convert the utc time to the yyyy/MM/dd
	*	@param {int} utc - the last modification time which is utc format.
	*	@return {string} - the time which is yyyy/MM/dd
	*/
	function utcToDateStr(utc){
	
		var date = new Date(utc*1000);
		var result = date.getFullYear() + '/' + (date.getMonth() + 1) + '/' + date.getDate() + ' ' + date.getHours() + ':' + date.getMinutes() + ':' + date.getSeconds();
		return result;
		
	}
	
	/*** 
	*	Author:Tim
	*	Desc:the function will convert byte to the suitable unit(max to GB).(Recursive)
	*	@param {int} bytes - file size.
	*	@param {int} unitInt - unit of file size (0 is kB, 1 is MB and 2 is GB).
	*	@return {string} - the file size which contains unit.
	*/
	function convertByte(bytes,unitInt){	
	
		
		if(typeof bytes=='object')
			bytes=bytes['#text']; //in order to process the non-number type.
	
		var unit=[' KB',' MB',' GB'];
	
		if(typeof unitInt=='undefined') unitInt=0;
	
		bytes=bytes/1024;
		var num = new Number(bytes);
		bytes = num.toFixed(1);
		
		if(bytes>1024)
			return convertByte(bytes,unitInt+1);
	
		return bytes + unit[unitInt];
	}
	
		
	
	/*** 
	*	Author:Tim/Tony
	*	Desc:the function will generate the layout of the folderview.(If user is searching, we will use the different layout )
	*	@param {object} data - data processed by getData
	*	@param {bool} isSearch - Whether user is searching?(Now, the variable is unused, because we do not set the search layout.)
	*	@return {string} - html of data.
	*/
	function createRow(index, data,isSearch){
	
		//var temp={name:null,modified:null,modifiedUTC:null,format:null,is_dir:null,bytes:null,size:null,path:null,nextPath:null};
		
		var name = data.name,
			imgWidth = 38, 
			imgHeight = 47;
		if(folderViewer.currentPath == '/')
		{
			if(name == 'storage')
				name = 'Media Drive';
			else if(name == 'sdcard')
				name = 'Media Drive Card';
		}
		
		var isSafari5 = (navigator.userAgent.indexOf('Safari') != -1 && navigator.userAgent.indexOf('Chrome') == -1) && /Version\/5/.test(navigator.userAgent);
		
		var trTagStr='<tr class="entryTr" format="'+data.format+'" fileName="'+data.name+'"  isdir="'+data.is_dir+'" modified="'+data.modified
						+'" path="'+data.path+'" nextPath="'+data.nextPath+'" modifiedUTC="'+data.modifiedUTC+'" bytes="'+data.bytes+'" size="'+data.size
						+'"  style="background-position: initial initial; background-repeat: initial initial; ">';
		var checkbox = '<td width="' + (isSafari5 ? 30 : 5) + 'px">' + t4u.ui.createCheckbox({name: data.name, format: data.format, isdir: data.is_dir, path: data.nextPath}) + '</td>';
		// clean the tooltip variable to disable it
		//var tooltip = '<span class="tooltip"><span class="middle">' + data.name + '</span></span>';
		var tooltip = '';
		var arrow = "<td width='35px' style='min-width:47px;'>" + t4u.ui.arrowButton + "</td>";

		var maxLength = 200;
		var itemname = (name.length > maxLength ? name.substring(0, maxLength) + ' ...' : name);
		var extname = data.name.substring(data.name.lastIndexOf('.'));
		var fileType = t4u.ui.getFileType(extname);
		var fileCss = data.format;
		//if(data.nextPath == '//storage/Personal' || data.nextPath == '/storage/Personal')
		//	fileCss = 'folderLocked';
		if (fileType != null)
			fileCss = fileType;

		var filepathName = data.nextPath;
		var filepath = t4u.encodeURI(filepathName.replace(/^\.?\/?\//ig, '/'));
		var fileName = '';
		if(isMobile()){
			////console.log('isMobile:'+isMobile());
			 
			if(data.format == 'photo')  //when file type is photo
			{
				fileName='<td><input name=""  type="checkbox" value=""><label>&nbsp;</label><a style="cursor:pointer" class="'+data.format
					+' photoviewer" rel="photoviewer" href="'+ filepath  +'" >'+name+'</a></td>'; 
			}
			else
			{
				fileName='<td><input name=""  type="checkbox" value=""><label>&nbsp;</label><a style="cursor:pointer" class="'+fileCss+'">'+name+'</a></td>';
			}
		}else{
			if(data.format == 'photo' || data.format == 'video' || data.format == 'audio')  //when file type is photo
			{
				var mediaType = data.format == 'audio' ? 'music' : data.format == 'photo' ? 'images' : data.format;
				t4u.bgProcess = true;
				
				var audioLink = '<a style="cursor:pointer" class="none ellipsis tt" href="javascript:void(0);" onclick="folderViewer.playSong($(this.parentNode.parentNode.parentNode));">'+itemname+ tooltip +'</a>';
				var fileNameArray = [];
				fileNameArray.push('<td style="padding-left:20px;"><div class="nameData" style="overflow:hidden;white-space: nowrap; min-width:90px; height:61px;"><div class="imgDivContainer">');
				if (typeof data.thumbnail !== 'string' || data.thumbnail == '')
				{
					var noThumbnailBG = getNoThumbnailBG(t4u.encodeURI(filepath), '' + imgWidth + 'x' + imgHeight);
					fileNameArray.push(noThumbnailBG);
					fileNameArray.push('<img style="width:' + imgWidth + 'px; height: ' + imgHeight + 'px;display: inline-block;margin: 10px 20px 0 0; ');
					fileNameArray.push(('display:none;') + '" id="file');
					fileNameArray.push(index + '" class="" src="');
					fileNameArray.push(('') + '" /></div>');
				}
				else
				{
					fileNameArray.push('<span class="imgContainer"><img id="file' + index + '" src="' + data.thumbnail + '" ');
					if (data.thumbnailSize)
					{
						fileNameArray.push('style="width:' + data.thumbnailSize.width + 'px; height: ' + data.thumbnailSize.height)
					 	fileNameArray.push('px;display: inline-block;margin: ' + data.thumbnailMargin.top + 'px 20px 0 0;" /></span></div>');
					}
					else
					{
						fileNameArray.push('style="width:' + imgWidth + 'px; height: ' + imgHeight + 'px;display: inline-block;margin: 10px 20px 0 0;" /></span></div>');
						var fsize = folderViewer.folders.size();
						var existent = folderViewer.files.get(index - fsize - 1);						
						photoView.fit('#file' + index, imgWidth, imgWidth, function(delta) {
							$.extend(existent, {'thumbnailSize': delta.size, 'thumbnailMargin': delta.margin});
						});
					}					 
				}
				
				if (data.format == 'audio')
					fileNameArray.push(audioLink);
				else if(data.format == 'photo')
					fileNameArray.push('<div class="photoLinkContainer"><a style="cursor:pointer" rel="photoviewer" class="none ellipsis tt '
							+ 'photoviewer' + '" href="' + filepath  +'" '
							+ ' >'+itemname+ tooltip +'</a></div>'); 
				else
					fileNameArray.push('<div class="photoLinkContainer"><a style="cursor:pointer" rel="photoviewer" class="none ellipsis tt '
							+ '" href="javascript:void(0);" '
							+ 'onclick="window.open(\'' + filepath + '\')">'+itemname+ tooltip +'</a></div>'); 
				fileNameArray.push('</div></td>');
				fileName = fileNameArray.join('');
			}
			else
			{
				if(data.is_dir)
					fileName='<td style="padding-left:20px;"><div class="nameData" style="overflow:hidden;white-space: nowrap;min-width:90px;"><a href="javascript:void(0);" style="cursor:pointer" class="tt ellipsis '+fileCss+'">' + itemname + tooltip + '</a></div></td>';
				else
					fileName='<td style="padding-left:20px;"><div class="nameData" style="overflow:hidden;white-space: nowrap;min-width:90px;"><a href="javascript:void(0);" onclick="folderViewer.downloadFile(\'' + filepath + '\',\'' + itemname + '\');" style="cursor:pointer;padding-left:58px;" class="tt ellipsis '+fileCss+'">' + itemname + tooltip + '</a></div></td>';
		}
		}
		
		if(!isSafari5)
		{
		if(typeof isSearch=='boolean' && isSearch){
			var time='<td width="130px"><div style="overflow:hidden;white-space: nowrap;min-width:130px;">'+data.modified+'</div></td>';
			var type='<td width="60px"><div style="overflow:hidden;white-space: nowrap;min-width:80px;">'+data.format+'</div></td>';
			var size='<td width="60px"><div style="overflow:hidden;white-space: nowrap;min-width:100px;">'+data.size+'</div></td>';	
		}else{	
			var time='<td width="130px"><div style="overflow:hidden;white-space: nowrap;min-width:130px;">'+data.modified+'</div></td>';
			var type='<td width="60px"><div style="overflow:hidden;white-space: nowrap;min-width:80px;">'+data.format+'</div></td>';
			var size='<td width="60px"><div style="overflow:hidden;white-space: nowrap;min-width:100px;">'+data.size+'</div></td>';	
		}
		}
		else
		{
			if(typeof isSearch=='boolean' && isSearch){
				var time='<td width="130px"><div style="overflow:hidden;white-space: nowrap;min-width:130px;">'+data.modified+'</div></td>';
				var type='<td width="130px"><div style="overflow:hidden;white-space: nowrap;min-width:130px;">'+data.format+'</div></td>';
				var size='<td width="130px"><div style="overflow:hidden;white-space: nowrap;min-width:130px;">'+data.size+'</div></td>';	
			}else{	
				var time='<td width="130px"><div style="overflow:hidden;white-space: nowrap;min-width:130px;">'+data.modified+'</div></td>';
				var type='<td width="130px"><div style="overflow:hidden;white-space: nowrap;min-width:130px;">'+data.format+'</div></td>';
				var size='<td width="130px"><div style="overflow:hidden;white-space: nowrap;min-width:130px;">'+data.size+'</div></td>';	
			}
		}
			
		return trTagStr+checkbox+fileName+time+type+size+arrow+'</tr>';
		
	}	
	
	/*** 
	*	Author:Tim
	*	Desc:the function will generate the path of the folderview.When user is in the root,he will look the 'home'.
	*	@param {string} path - the whole path of that file.
	*	@return {string} - html of path which will contian the hyperlink.
	*/
	function setPathLink(path){
	
		var root= t4u.format('<a class="pathLink" href="javascript:void(0);">{0}</a>','Home');
	
		if(path=='/')
		{
			return '';
		}
	
		var str='';
		var pathArr=path.split('/');
		for(var i=0;i<pathArr.length;i++){
			if(pathArr[i]=='') continue;		
			var tag='';
			pathArr=path.split('/');
			for(var j=0;j<=i;j++){
				tag=tag+'/'+pathArr[j];
				tag=tag.replace(/\/+/mg,'/');
			}
			if(i == 1)
			{
				if(pathArr[1] == 'storage')
				{
					pathArr[1] = 'Media Drive';
				}				
				else if(pathArr[1] == 'sdcard')
				{
					pathArr[1] = 'Media Drive Card';
				}
			}
			str+=t4u.format(' >> <a class="pathLink" tag="{0}" href="javascript:void(0);">{1}</a>',tag,pathArr[i]);
		}
	
		return root+str;
	}

	/*** 
	*	Author:Tim
	*	Desc:Initial the folderView.
	*	@param nothing
	*	@return nothing
	*/
	function initialize(){
		
		
		folderViewer.isAllData=false;
		$('.entryTr').remove();       //we need to remove all the original data in the table.
		folderViewer.index = 1;
		
		topBtn(false);
		updateMenu();

		hideMusicBar();
		setTableTitleLang();	
	}
	
	/***
	*	Author: tony		
	*	Desc: set Scotti capacity info
	*	@param nothing
	*	@return nothing
	*/
	function setNimbusCapacity(){
		var nRetryCnt = 0;
		var getNimbusInfo=function()
		{
			t4u.nimbusApi.setting.getNimbusInfo(function(data){
				if(t4u.nimbusApi.getErrorCode(data) == 0)
				{
					var totalCapacity =0;
					var freeCapacity = 0;
					
					if(typeof data.nimbus.storages != 'undefined')
					{
						if(data.nimbus.storages.storageitem.length>0)
						{					
							for(var i =0; i < 1 /*data.nimbus.storages.storageitem.length*/ ;i++)
							{						
								if(typeof data.nimbus.storages.storageitem[i].total=='object')
									totalCapacity += data.nimbus.storages.storageitem[i].total['#text'];
								else
									totalCapacity += data.nimbus.storages.storageitem[i].total;
									
								if(typeof data.nimbus.storages.storageitem[i].free =='object')
									freeCapacity += data.nimbus.storages.storageitem[i].free['#text'];
								else
									freeCapacity += data.nimbus.storages.storageitem[i].free;
							}
							
						}else if(typeof data.nimbus.storages.storageitem.total !='undefined'){
							
							if(typeof data.nimbus.storages.storageitem.total=='object')
									totalCapacity += data.nimbus.storages.storageitem.total['#text'];
							else
								totalCapacity += data.nimbus.storages.storageitem.total;
								
							if(typeof data.nimbus.storages.storageitem.free =='object')
								freeCapacity += data.nimbus.storages.storageitem.free['#text'];
							else
								freeCapacity += data.nimbus.storages.storageitem.free;
						}
					}
					totalCapacity = totalCapacity*4;
					freeCapacity = freeCapacity*4;
					totalCapacity = convertByte(totalCapacity,1);
					freeCapacity = convertByte(freeCapacity,1);
					$('#spanCapacity').text(totalCapacity);
					$('#spanAvailable').text(freeCapacity);
					$('#setting-check-firmware-upgrade').text(data.nimbus.fwver); 

					if(typeof data.nimbus.storages.storageitem.length != 'undefined')
						for(var i=0; i< data.nimbus.storages.storageitem.length; i++)
						{
							if(data.nimbus.storages.storageitem[i].fs == 'fat32')
							{
								var mountpoint = data.nimbus.storages.storageitem[i].mountpoint;
								mountpoint = mountpoint.replace('/mnt', '');
								if($.inArray(mountpoint, folderViewer.fat32System) == -1)
									folderViewer.fat32System.push(mountpoint);
							}
						}
				}
			},
			//fail
			function(data){
				if(nRetryCnt < 3)
				{
					nRetryCnt++;
					getNimbusInfo.call(this);
				}
			});
		}
		getNimbusInfo.call();
	}
	
	
	/*** 
	*	Author:Tim
	*	Desc:the prime of funciton sort_by.
	*	@param {object} a - input data.
	*	@return {string} - If type of a is string, a will to upper case, or a does not change.
	*/
	function sortCondition(a){
	
		if(typeof a=='string')
			return a.toUpperCase();
			
		return a;
	}
	
	/*** 
	*	Author:Tim
	*	Desc: set the folderView's table multi-language.
	*	@param nothing
	*	@return nothing
	*/
	function setTableTitleLang(){
	
		var title=$('.sort').parent().children('p');
		for(var i=0;i<title.length;i++){
			title.eq(i).text(multiLang.search(title.eq(i).text()));
		}
	
	}
	
	function getFileAbsolutePath(item)
	{
		var filepathName = item.nextPath;
		return filepathName.replace(/^\.?\/?\//ig, '/');
	}
	
	/*** 
	*	Author:Tim
	*	Desc:the function will generate the view of the folderview.When user is in the root,he will look the 'home'.
	*	@param {string} path - the whole path of that dir.
	*	@param {string} sort_field - the field which user want to sort.
	*	@param {bool} sort_asc - asc or desc?
	*	@return nothing
	*/
	function setup(path,sort_field,sort_asc){
	
		//init
		// if(!folderViewer.isAppend)
		// 	initialVal();
			
		if(folderViewer.isAllData){ 
		
			return;
		}
		
		// t4u.nimbusApi.fileExplorer.getDir(getData,{start:folderViewer.index,count:folderViewer.length,path:path});
		readData(path, folderViewer.index, folderViewer.length);
		
		if(path=='/' )  //in order to get the root.
			path='';
	}
	
	function readData(path, index, length) {
		folderViewer.isLoading = true;
		t4u.nimbusApi.fileExplorer.getDir(function(data) {
			if(t4u.nimbusApi.getErrorCode(data) == 0)
			{
				var items = t4u.nimbusApi.getRemoteItems(data);
				
				if(typeof data.nimbus.items.entry == 'object' && typeof data.nimbus.items.entry.length == 'undefined' && folderViewer.currentPath == '/')
				{
					folderViewer.setup('/storage', true);
					return;
				}
				
				folderViewer.addItem(data);

				if (items.length == folderViewer.length)
				{
					readData(path, index + length, length);
				}
				else
				{
					t4u.unblockUI();
					folderViewer.sort('name', false);
					folderViewer.isLoading = false;
					$('.sort').eq(0).show();
					$('.sort').eq(0).attr('tag', 'selected');
				}
			}
			else
				folderViewer.loadingDataFromServer(false);
		},function(data){
		},{
			start: index,
			count: length,
			path: path
		});
		t4u.bgProcess = true;
	}

	function showItems(endIndex)
	{
		folderViewer.loadingDataFromServer(false);
		var table = $('#divFolderView table');
		var length = 0;
		if ($.isNumeric(endIndex))
		{
			folderViewer.index = 1;
			length = endIndex;
		}
		else
			length = folderViewer.length;
		
		for (var i = 0; i < length; i++)
		{
			var index = folderViewer.index++;
			var item = index <= folderViewer.folders.size() ?
							folderViewer.folders.get(index - 1) :
							folderViewer.files.get(index - folderViewer.folders.size() - 1);
			if (item == null)
				break;

			table.append(createRow(index, item));
			var data = item;
			if(data.format == 'photo' || data.format == 'video' || data.format == 'audio')
			{
				if (!data.thumbnailSize)
				{
					var fsize = folderViewer.folders.size();
					var existent = folderViewer.files.get(index - fsize - 1);						
					photoView.fit('#file' + index, 38, 38, function(delta) {
						$.extend(existent, {'thumbnailSize': delta.size, 'thumbnailMargin': delta.margin});
					});
				}	
		}
		}
		
				var photos = [];
				for (var i = 0; i < folderViewer.files.size(); i++)
				{
					var item = folderViewer.files.get(i);
					if (item.format == 'photo')
					{
						var href = getFileAbsolutePath(item);
						var title = item.name;
						photos.push({'title': title, 'link': href, 'settings': null});
					}
				}

		$('.photoviewer').colorbox({
			rel:'folderview',
			arrowKey: false,
			loop: false,
			slideshow: true,
			slideshowAuto: false,
			opacity: 1,
			fitScreen: true,
			onDatasource: function(settings, index) {
				if (index >= 0 && index < photos.length)
					photos[index]['settings'] = settings;
				return photos;
			},
			onGetCount: function() {
				return photos.length;
			}
		});
		
		//folderViewer.counter+=inifiniteScrollSize;
		if ($.isNumeric(endIndex))
		{
		setTableWidthByTable($('#divFolderView .dropBoxDivContainer'), $('#dropBoxDivTitle'));
		setTableWidthByTable($('#divFolderView .dropBoxDivContainer'), $('#divFolderView .dropBoxDivContainer'));//Andrew設定內容也要有width
	}
	}

	function setGoback() {
		var $goBack = $('#divBackButton');
		var $folderPath=$('#folder-path');
		if (folderViewer.history.length > 0){
			$goBack.show();
			$folderPath.attr('style','display:block; margin-left:10px; padding-left: 0px; padding-top:20px; width:300px;text-align:left;');
			}
		else
			$goBack.hide();

		var events = $goBack.data('events');
		if (typeof events === 'undefined')
			$goBack.bind('click', folderViewer.goBack);		
	}
	
	var folderViewer = {
		index: 0,
		length: inifiniteScrollSize,
		copyPath:{path: '', isDir: false, size: 0},     //in order to copy and paste.
		copySelected:[],//copy selected
		isAppend:false,
		isAllData:false,
		currentPath: '/',
		isLoading: false,
		history: [],
		folders: null, 
		files: null,
		finder: null,
		fat32System:[],
		fileToDownload:'',

		addItem: function(data)
		{
			var items = t4u.nimbusApi.getRemoteItems(data);
			for (var i = 0; i < items.length; i++)
			{
				var item = getFolderData(items[i]);
				if (item == null)
					continue;
				if (item.is_dir)
					this.folders.add(item);
				else
					this.files.add(item);
			}
		},

		find: function(keyword)
		{
			if (keyword == '')
			{
				this.goBack();
				return;
			}
			folderViewer.loadingDataFromServer(true);
			if (this.finder == null)
			{
				this.finder = new t4u.FolderFinder({
					onDone: function() {
						this.clear();
						if(folderViewer.folders.size() + folderViewer.files.size() == 0)
						{
							$('#div-no-records-found-file').show();
						}
						folderViewer.sort('name', false);
						folderViewer.isLoading = false;
					},
					onFound: function(data, index, length)
					{
						var items = t4u.nimbusApi.getRemoteItems(data);
						folderViewer.addItem(data);

						// if (index <= 1)
						// 	showItems();

						// this.collection.clear();
					}
				});
			}

			folderViewer.history.push(folderViewer.currentPath);
			setGoback();
			this.clear();
			this.folders.clear();
			this.files.clear();
			this.finder.clear();
			folderViewer.isLoading = true;
			this.finder.find(keyword, 1, folderViewer.length);
		},

		/*** 
		*	Author:Tim
		*	Desc: To set the folderViewer.isAppend.
		*	@param {bool} isAppend - the value which user wants.
		*	@return nothing
		*/
		appendData:function(isAppend){
			this.isAppend=isAppend;
		},
		/*** 
		*	Author:Tim
		*	Desc:the function will generate the view of the folderview.When user is in the root,he will look the 'home'.
		*	@param {string} path - the whole path of that dir.
		*	@return nothing
		*/
		setup:function(path, noHistory){
			t4u.bgProcess = false;
			t4u.abort();
		
			folderViewer.loadingDataFromServer(true);
			$('#div-no-records-found-file').hide();
		
			if (this.folders == null)
			{
				this.folders = new t4u.Collection();
				this.files = new t4u.Collection();
			}
			else
			{
				this.folders.clear();
				this.files.clear();
			}
		
			if(typeof path=='undefined')
				path='/';
				
			/*
			if (path.indexOf('/storage/Personal') != -1 && !t4u.SS.isLogin())
			{
				t4u.showMessage(multiLang.search('Permission Denied'));
				this.setup('/storage');
				return false;
			}
			*/

			if (path == '/')
				this.history.length = 0;
			else if (!noHistory && this.history[this.history.length - 1] != path)
				this.history.push(this.currentPath);

			setGoback();

			folderViewer.goTo(path);
			initialize();
			this.currentPath = path;
			setup(path);    //the path is always the '/',beacause it need to get all data.
			
			
			t4u.scroll.bind($('#divFolderView'), function(){
				if (folderViewer.isLoading)
					return;

				var size = folderViewer.folders.size() + folderViewer.files.size();
				if (folderViewer.index == size)
				{				
					t4u.scroll.unbind();
					return;
				}
				else
					showItems();
			});
		},
		
		/*
		* Reload current folder
		*/
		reload: function(noHistory) {
			this.setup(this.currentPath, noHistory);
		},
		
		/*** 
		*	Author:Tim
		*	Desc:the function will set the full path in the $('.path').
		*	@param {string} path - the whole path of that dir.
		*	@return nothing
		*/
		goTo:function(path){
			$('.path').html(setPathLink(path));
			showTopBtn(path != '/');
			nowPath=path;
		},
		
		goBack: function() {
			$('#searchContainer .goBack').blur();
			var self = folderViewer;
			if (self.history.length > 0)
			{
				var path = self.history[self.history.length - 1];
				self.history.length -= 1;
				folderViewer.setup(path, true);
			}

			if (self.history.length <= 0)
			{
				var $goBack = $('#divBackButton');
				$goBack.hide();
				$goBack.unbind('click', folderViewer.goBack);				
			}			
		},
		resetData:function(){
			var index = folderViewer.index;
			this.clear();
			showItems(index > folderViewer.length ? index - 1 : folderViewer.length);
		},
		/*** 
		*	Author:Tim
		*	Desc:the function will sort the all view, and the priority of folder(s) is bigger than file(s).
		*	@param {string} path - the whole path of that dir.
		*	@param {bool} asc - asc or desc?
		*	@return nothing
		*/
		sort:function(field, reverse){
			if (folderViewer.folders.sort(field, reverse) && folderViewer.files.sort(field, reverse))
			{
				var index = folderViewer.index;
				this.clear();
				showItems(index > folderViewer.length ? index - 1 : folderViewer.length);
			}
			//setTableWidthByTable($('#divFolderView .dropBoxDivContainer'), $('#dropBoxDivTitle'));
		},
		/***
		*	Author: tony		
		*	Desc: init Scotti capacity
		*	@param nothing
		*	@return nothing
		*/ 
		initNimbusCapacity:function(){ 
			setNimbusCapacity();
		},

		playSong: function(selector) {
			audioPlaylist.clearAndStop();
			var audioFiles = selector.parent().find("tr[format='audio']"),
				index = audioFiles.index(selector);
			for(var i=0; i<audioFiles.length; i++)
			{
				audioView.playInFolderView($(audioFiles[i]).attr("nextPath").replace(/^\/?\//ig, '/'));
			}
			function checkAndPlay()
			{
				if(AudioPlayer.playlist.length == audioFiles.length)
				{
					audioPlaylist.stopAndPlay(index);
				}
				else
				{
					setTimeout(function(){
						checkAndPlay();
					}, 500);
				}
			}
			setTimeout(function(){
				checkAndPlay();
			}, 500);
			
		},
		
		clear: function(){
			initialize();
			$('.entryTr').remove();
		},
		/***
		*	Author: Steven		
		*	Desc: check whether the destination file system is fat32 or not
		*	@param {string} path - the path of the directory
		*	@return {bool}
		*/
		fileSystemIsFat32: function(path){
			for(var i=0; i< this.fat32System.length; i++)
			{
				if(path.indexOf(this.fat32System[i]) != -1)
					return true;
			}
			return false;
		},
		loadingDataFromServer: function(loading){
			if(loading)
			{
				$('#div-dynamicLoading-file').show();
				$('#divFolderView').hide();
			}
			else
			{
				$('#div-dynamicLoading-file').hide();
				$('#divFolderView').show();
			}
		},
		downloadFile: function(filepath, filename){
			var extension = filename.substring(filename.lastIndexOf('.')).toLowerCase();
			if(extension == '.pdf')
			{
				window.open(filepath);
				return;
			}		
			$('#download-file-name').html(multiLang.search('File Name:') + filename);
			t4u.blockUI($('#div-download-file'));
			folderViewer.fileToDownload = filepath;
			
			$('#btnDownloadCancel').click(function(){
				t4u.unblockUI();
				$('#btnDownloadCancel').unbind('click');
			});
			
			$('#btnDownloadOK').click(function(){
				var iframeId = 'iframeid0';
				$('<iframe id="' + iframeId + '"></iframe>').appendTo('body').hide();
				$('#' + iframeId).load(function(){
					$(this).remove();
				}).attr( "src", folderViewer.fileToDownload.replace(/^\.?\/\//ig, '') + '?dn' );
				
				t4u.unblockUI();
				$('#btnDownloadOK').unbind('click');
			});
		}
	};
	
	window.folderViewer = folderViewer;
})(jQuery);