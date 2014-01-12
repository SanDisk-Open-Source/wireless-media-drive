;(function($){
	
	var container = '#videoListItems';

	/***
		Author: tony		
		Desc: init video list
	*/ 
	function videoSetup(data)
	{
		var videoContainer = $(container);
		var entryObj = getObjEntry(data);

		var allData = [];
		var html = videoContainer.html();
		allData.push(html == null ? '' : html);
		if(entryObj.length >0)
		{
			for(var i = 0; i < entryObj.length; i++)
			{
				if (typeof entryObj[i].title == 'undefined')
					continue;
				var index = videoView.index++;
				allData.push(createVideo(entryObj[i], index));
				photoView.fit('#videoImg' + index, 145, 110);
			}
		}else{
			if (typeof entryObj.title == 'undefined')
				return;
			var index = videoView.index++;
			videoView.allData = true;
			allData.push(createVideo(entryObj, index));
			photoView.fit('#videoImg' + index, 145, 110);
		}
		videoContainer.html(allData.join('')); 
		
		initClick();
		selectVideo();
		videoView.isLoading = false;
	}
	 
	
	 
	/***
		Author: tony		
		Desc: init button click
	*/ 
	function initClick()
	{
		//double click
		$('.divVideo').off('click');
	 //    $('.divVideo').click(function(){
		// 	// t4u.blockUI();
		// 	var videoPath = $(this).attr('href');
		// 	location.href = videoPath;
		// 	setTimeout('t4u.unblockUI();',10000);
		// });  
		
		//video hover function
		$('.divVideo').off('hover');
		$('.divVideo').hover(function() { 
			var SelectID = "#divVideoSelect" + $(this)[0].id.replace("divVideo", "");   //blue border div ID 
			$(SelectID).css("display", ""); 
		}, function() {  
			if(!$(this).hasClass("VideoSelected"))
			{
				var SelectID = "#divVideoSelect" + $(this)[0].id.replace("divVideo", "");   //blue border div ID
				$(SelectID).css("display", 'none');
			}
		});

		var selection = $('#videoListItems li label .cbSelection');
		selection.die('click');
		selection.live('click', function(e){
			eventClick('', this.parentNode.childNodes[0], $(this));
			e.stopPropagation();
		});
		
		$('#btnVideoHintOK').click(ShowHintOK);
		$('.hideHint').click(function() {
			if($('.hideHint').hasClass('checked')) {
				$('.hideHint').removeClass('checked');
				$('#cbHideHint').attr('checked', false);
			}
			else {
				$('.hideHint').addClass('checked');
				$('#cbHideHint').attr('checked', true);
			}
		});
	}
	
	function appendZero(value)
	{
		return (value < 10 ? "0" + value : "" + value);
	}
	
	function formatTime(second)
	{
		var sec, minute, hour, ret;
		sec = second - Math.floor(second/60)*60;
		minute = Math.floor(second/60) - Math.floor(second/3600)*60;
		hour = Math.floor(second/3600);
		ret = (hour == 0 ? "" : ((appendZero(hour) == "0" ? "" : appendZero(hour)) + ":")) + appendZero(minute) + ":" + appendZero(sec);
		return ret;
	}
	
	/***
		Author: tony		
		Desc: bind video data for html tag
	*/ 
	function createVideo(entryData, nIndex)
	{ 
		var result = '',
			title = getObjData(entryData.title),
			addMarginTop='',
			duration=getObjData(entryData.duration),
			checkbox = t4u.ui.createCheckbox({
			name: entryData.title,
			path: entryData.path,
			index: nIndex,
			thumbnail: (typeof entryData.thumbnail == 'object') ? '' : entryData.thumbnail
		});
		result = '<li>'; 
		result +='<a target="_blank" border="0" ID="divVideo'+ nIndex +'" class="divVideo" videoName="'+title+'" href="'+ t4u.encodeURI(entryData.path) +'" path="'+ t4u.encodeURI(entryData.path) +'" style="cursor:pointer;" > '; 		
		if(entryData.thumbnail == "(NULL)" || typeof entryData.thumbnail != 'string'){
			addMarginTop="margin-top:3px;";
			result += getNoThumbnailBG(entryData.path, '145x110') //'<img id="videoImg'+ nIndex +'" src="'+ virtualPath + '/images/videoPlay.png' +'" border="0"  />'; 
			}
		else
		        result += '<img id="videoImg'+ nIndex +'" src="'+ entryData.thumbnail +'" border="0"  />'; 
		result += '<div id="divVideoSelect'+ nIndex +'" class="divVideoSelect" style="position: relative; display:none;"> <!--<div class="addNewBlueBorder" style="'+ addMarginTop +'" ></div>--></div></a>'
		result += '<div class="title" style="white-space: nowrap;">' + checkbox + showShortVideoTitle(title) +'<p>'+ formatTime(Math.floor(duration/1000)) + '</p></div></div>';
		result += '</li>'; 
		
		return result;
	}
	
	function showShortVideoTitle(videoTitle)
	{
		var newVideoTitle = '<div style="width:133px;" class="shortInfor" title = "' + videoTitle + '">' + videoTitle + '';
		return newVideoTitle;
	}
	
	/***
		Author: tony		
		Desc: video one Click event 
	*/	
	function selectVideo()
	{ 
		$('.divVideo').click(function(){ 
		
			topBtn(true);
			var nowSelectIndex = $(this)[0].id.replace("divVideo", "");
			var SelectID = "#divVideoSelect" + nowSelectIndex;   //blue border div ID
						
			if($(this).hasClass("VideoSelected") && (isShift || isCtrl))//check this div is select
		    {
				//cancel select 
				$(this).removeClass('VideoSelected');
				$(SelectID).css("display","none");
			}
			else if(!isShift || videoView.lastSelect ==0)	//keyboard click ctrl or none 
			{ 
				if(!isCtrl)
				{
					//cancel select
					$('.divVideo').removeClass('VideoSelected');
					$('.divVideoSelect').css("display","none");
				}
				
				//set
				$(this).addClass('VideoSelected'); 
				$(SelectID).css("display",""); 
			}
			else //keyboard click shift
			{
				var startIndex=0;
				var endIndex=0;
				
				if(Number(videoView.lastSelect) > Number(nowSelectIndex))
				{
					startIndex = nowSelectIndex;
					endIndex = videoView.lastSelect; 
				}
				else
				{
					startIndex = videoView.lastSelect;
					endIndex = nowSelectIndex;
				}
				
				//cancel select
				$('.divVideo').removeClass('VideoSelected');
				$('.divVideoSelect').css("display","none");
				
				//set select 
				for(startIndex; Number(startIndex) <= Number(endIndex) ; startIndex ++)
				{					 
					$("#divVideo" + startIndex).addClass('VideoSelected'); 
					$("#divVideoSelect" + startIndex).css("display","");  
				}
			}
			
			if(videoView.lastSelect == 0 ||  !isShift)	//when isn't click shift or no selected video ,save last select index
				videoView.lastSelect = nowSelectIndex; //set last select index
			
			if($('.VideoSelected:visible').length==0)
				topBtn(false);
			
			$('.divVideo').attr('tag','');
			$('.VideoSelected:visible').attr('tag','clicked');
			if(t4u.notShowHint != 'true')
			{
				$('#cbHideHint').attr('checked', true);
				t4u.blockUI($('#div-video-hint'));
			}
		});
	}
	
	function ShowHintOK()
	{
		var bNotShowAgain = ($('#cbHideHint').attr('checked') == 'checked');
		if(bNotShowAgain)
		{
			t4u.notShowHint = 'true';
			$.cookie('NoVideoHint', 'true', { expires: 365 * 100 });
		}
		t4u.unblockUI();
	}
	
	function showItems() {
		var videoContainer = $(container);
		var collection = videoView.finder.collection;
		if (collection.size() == 0)
			return;

		var allData = [];
		var html = videoContainer.html();
		allData.push(html == null ? '' : html);
		
		for (var i = 0; i < videoView.length; i++)
		{
			var index = videoView.index++;
			var item = collection.get(index - 1);
			if (item == null)
			{
				videoContainer.html(allData.join('')); 
				initClick();
				selectVideo();
				return;
			}

			allData.push((createVideo(item, index)));
			photoView.fit('#videoImg' + index, 145, 110);
		}
		videoContainer.html(allData.join('')); 

		initClick();
		selectVideo();
	}
	
	var videoView = { 
		index: 1,
		length: inifiniteScrollImgSize,
		lastSelect : 0,
		allData: false,
		isLoading: false,
		finder: null,

		initialize: function() {
			this.index = 1;
			this.allData = false;
			$(container).html('');
		},

		find: function(keyword) {
			$('#div-no-records-found-video').hide();
			if (this.finder == null)
			{
				this.finder = new t4u.MediaFinder('video', {
					onDone: function() {
						videoView.isLoading = false;
						videoView.loadingDataFromServer(false);	
						//no data found
						if(videoView.finder.collection.get(0) == null)
						{
							$('#div-no-records-found-video').show();
						}
					},
					onFound: function(data, index, length) {
						if (index <= 1)
							showItems();
					}
				})
			}
			this.clear();
			this.initialize();
			videoView.loadingDataFromServer(true);
			this.finder.clear();
			this.finder.find(keyword, 1, videoView.length);
			t4u.scroll.bind($('#content'), function(){
				showItems();
			});
		},

		setup : function(){
			this.initialize();			
			videoView.loadingDataFromServer(true);
			$('#div-no-records-found-video').hide();
			this.load(videoView.index, videoView.length);
			t4u.scroll.bind($('#content'), function(){
				if (videoView.isLoading)
					return;

				if (videoView.allData)
				{
					t4u.scroll.unbind();
					return;
				}
				t4u.bgProcess = true;
				videoView.isLoading = true;
				videoView.load(videoView.index, videoView.length);
			});
		},
			
		load: function(start, length) {
			t4u.nimbusApi.fileExplorer.getMediaList(
				function(data){
					if(t4u.nimbusApi.getErrorCode(data) == 0)
						videoSetup(data); 
					else
					{
						if(data.nimbus.errcode != 116)
						{
						var msg = getObjData(data.nimbus.errmsg);
						t4u.showMessage(msg);
					}					
					}					
					videoView.loadingDataFromServer(false);	
				}, 
				function(data){
					videoView.loadingDataFromServer(false);
				},
				{'mediaType':[{'start':start,'count':length,'type':'video'}],'sort':[{'order':'asc','field':'title'}]}, 
				{filter:[]});
		},
		
		clear: function(){
			$(container).html('');
		},
		loadingDataFromServer: function(loading) {
			if(loading)
			{
				$('#div-dynamicLoading-video').show();
				$('#videoListItems').hide();
			}
			else
			{
				$('#div-dynamicLoading-video').hide();
				$('#videoListItems').show();
			}
		}
		 
	};
	window.videoView = videoView;
	
})(jQuery);
