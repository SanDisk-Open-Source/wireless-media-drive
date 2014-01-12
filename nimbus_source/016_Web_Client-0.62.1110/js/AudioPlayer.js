var loopModes = {
		NoLoop : {value:0}, 
		LoopOne: {value:1}, 
		LoopList: {value:2}
};

var AudioPlayer = {
	playlist:[],
	
	ShuffleList:[],
	OriginalList:[],
	
	playindex:0,
	
	Shuffle: false,
	
	ToggleShuffle:function() {
		this.Shuffle = !this.Shuffle;
		this.setShuffleBtnStatus();
		var jp = $(this.jPlayerSelect);
		if (this.Shuffle)
		{
			// switch to shuffle mode now, so we re-order list.
			// and switch to play from list one.
			this.ShuffleList = [];
			for (var i=0;i<this.playlist.length;i++)
				this.ShuffleList.push(this.playlist[i]);
			
			var swaptimes = this.playlist.length;
			var indexlength = this.playlist.length;
			for (var i=0;i<swaptimes;i++){
				var swapx = Math.floor(Math.random() * indexlength);
				var swapy = Math.floor(Math.random() * indexlength);
				if (swapx != swapy){
					var temp = this.ShuffleList[swapy];
					this.ShuffleList[swapy] = this.ShuffleList[swapx];
					this.ShuffleList[swapx] = temp;
				}
			}
			this.OriginalList = this.playlist;
			this.playlist = this.ShuffleList;
			
			$(this.cssSelector.shuffleBtn).addClass(this.cssSelector.shuffleOnClass);			
			// start to shuffle table tr
			if(mode == "nowPlayingList")
			$('#getting-NowPlayinglists').click();
			
			this.onplaylistchange(this.playlist);
			this.playindex = 0;
			jp.jPlayer("stop");
			jp.jPlayer("setMedia", this.preventCache(this.playlist[this.playindex]));
			if (this.isplaying) {
				this.play();
			}
		} 
		/*else {
			this.playlist = this.OriginalList;
		}*/
		
			

	},
	
	ToggleLoop: function() {
		if (this.loopMode == loopModes.NoLoop) {
			this.loopMode = loopModes.LoopList;
		} else if (this.loopMode == loopModes.LoopOne) {
			this.loopMode = loopModes.NoLoop;
		} else if (this.loopMode == loopModes.LoopList) {
			this.loopMode = loopModes.LoopOne;
		}
		this.setLoopBtnStatus();
	},
		
	loopMode: loopModes.NoLoop,
	
	stop:function() {
		if (this.inited)
		{			
			var jp = $(this.jPlayerSelect);
			jp.jPlayer("stop");
		} else {
			this.shouldplayAfterInit = false;
		}
	},
	
	isplaying: false,
	isPaused: false,
	
	playStatus:function() {
		if (this.inited)
		{			
			var jp = $(this.jPlayerSelect);
			return jp.jPlayer.status.srcSet;
		}
		else
			return false;	
	},
		
	play:function() {
		if (this.inited)
		{			
			if(this.playindex != -1)
			{
				this.onplay(this.playlist[this.playindex]);
				this.isplaying = true;
				var jp = $(this.jPlayerSelect);
				jp.jPlayer("play");
			}
		} else {
			this.shouldplayAfterInit = true;
		}
	},
	
	shouldplayAfterInit : false,
	
	pause:function() {
		if (this.inited)
		{			
			var jp = $(this.jPlayerSelect);
			jp.jPlayer("pause");
			this.isplaying = false;
		}
	},
	
	next:function() {
		this.playindex++;
		if (this.playindex >= this.playlist.length)
		{
			if (this.Shuffle) {
			    if(mode == "nowPlayingList") {
				this.playindex = 0;
				$('#getting-NowPlayinglists').click();
				this.play();
				return;
			    }
			}

			if (this.loopMode != loopModes.LoopList)
			{
				this.playindex--; // roll back .
				if(mode == "nowPlayingList")
					$('#getting-NowPlayinglists').click();
				return;
			}

			this.playindex = 0;
		}
		
		if (this.inited)
		{			
			var jp = $(this.jPlayerSelect);
			jp.jPlayer("stop");
				jp.jPlayer("setMedia", this.preventCache(this.playlist[this.playindex]));
			t4u._debug(this.playlist[this.playindex].path);
			this.onplay(this.playlist[this.playindex]);			
			jp.jPlayer("play");
			this.isplaying = true;
		}
	},
	preventCache:function(song){
		if(song.path.indexOf('t=') < 0)
		{			
			var musicFormat=song.path.split('.');
			musicFormat=""+musicFormat[musicFormat.length-1];
			if(song.path.indexOf('/') == 0)
			{
				song.path = t4u.encodeURI(song.path);
			}
			if(song[musicFormat].indexOf('/') == 0)
			{
				song[musicFormat] = t4u.encodeURI(song[musicFormat]);
			}
		
			song.path = song.path + '?t=' + ( new Date() ).getTime();			
			song[musicFormat] = song.path + '?t=' + ( new Date() ).getTime();
		}
		return song;
	},
	prev:function() {
		this.playindex--;
		if (this.playindex < 0)
		{
			if (this.loopMode != loopModes.LoopList)
			{
				this.playindex = 0;
				return;
			}
			this.playindex = this.playlist.length;
		}
		
		if (this.inited)
		{
			var jp = $(this.jPlayerSelect);
			jp.jPlayer("stop");
			jp.jPlayer("setMedia", this.preventCache(this.playlist[this.playindex]));
			this.onplay(this.playlist[this.playindex]);			
			jp.jPlayer("play");
			this.isplaying = true;
		}	
	},
	inited: false,
	
	setLoopBtnStatus:function() {
		if (this.loopMode == loopModes.NoLoop) {
			$(this.cssSelector.loop).removeClass(this.cssSelector.loopOneClass+ ' '+this.cssSelector.loopListClass);
		} else if (this.loopMode == loopModes.LoopOne) {
			$(this.cssSelector.loop).addClass(this.cssSelector.loopOneClass);
			$(this.cssSelector.loop).removeClass(this.cssSelector.loopListClass);
		} else if (this.loopMode == loopModes.LoopList) {
			$(this.cssSelector.loop).removeClass(this.cssSelector.loopOneClass);
			$(this.cssSelector.loop).addClass(this.cssSelector.loopListClass);
		}
	},
	
	setShuffleBtnStatus:function() {
			$(this.cssSelector.shuffleBtn).removeClass(this.cssSelector.shuffleOnClass);
	},
	
	cssSelector:{ // * denotes properties that should only be required when video media type required. _cssSelector() 	would require changes to enable splitting these into Audio and Video defaults.
					videoPlay: ".jp-video-play", // *
					play: ".musicPlay",
					pause: ".musicPause",
					stop: ".musicStop",
					seekBar: "#musicProgressBar",
					playBar: ".jp-play-bar",
					mute: ".jp-mute",
					unmute: ".jp-unmute",
					volumeBar: ".jp-volume-bar",
					volumeBarValue: ".jp-volume-bar-value",
					volumeMax: ".jp-volume-max",
					currentTime: ".jp-current-time",
					duration: ".jp-duration",
					fullScreen: ".jp-full-screen", // *
					restoreScreen: ".jp-restore-screen", // *
					//repeat: ".musicLoop",
					repeatOff: ".jp-repeat-off",
					gui: ".jp-gui", // The interface used with autohide feature.
					noSolution: ".jp-no-solution", // For error feedback when jPlayer cannot find a solution.
					seekBarBtn: '#musicBarBtn',
					seekBarBkg: '.musicProgressBar',
					seekBarLeft: '#musicProgressRateBar',
					songName : '#songname',
					albumName :'#albumname',
					albumPhoto : '#albumphoto',
					albumBack : '.albumback',
					next: '.musicNext',
					prev: '.musicPrev',
					loop: '.musicLoop',
					loopOneClass: 'musicLoopPressTwice',
					loopListClass: 'musicLoopPressOne',
					shuffleBtn: '.musicShuffle',
					shuffleOnClass: 'musicShufflePress',
					volumeBarBkg : '#volumeProgressBar div',
					volumeBarUp : '#volumeProgressRateBar',
					volumeBarBtn : '#volumeBarBtn',
					musicInfo : '.musicInfoTouch',
					musicBarInfo : '.musicBarInfo'
				},
	seekBar: function(e) { // Handles clicks on the seekBar
					if($(this.cssSelector.seekBarBkg)) {
						var offset = $(this.cssSelector.seekBarBkg).offset();
						var x = e.pageX - offset.left;
						var w = $(this.cssSelector.seekBarBkg).width();
						var p = 100*x/w;
						var jp = $(this.jPlayerSelect);
						jp.jPlayer("playHead", p);
					}
				},
	volumeBar: function(e) {
		var offset = $(this.cssSelector.volumeBarBkg).offset();
		h = $(this.cssSelector.volumeBarBkg).height()-60;
		y = h - e.pageY + offset.top+25;
		if (y >= h) { y = h; }
		if (y < 0) { y = 0; }
		this.SetVolumeBar(y/h);
		var jp = $(this.jPlayerSelect);
		jp.jPlayer("volume", y/h);
	},
	
	SetVolumeBar : function (p) {
		h = $(this.cssSelector.volumeBarBkg).height()-60;
		if (p > 0.95) {
			$(this.cssSelector.volumeBarBtn).hide();
			$(this.cssSelector.volumeBarUp).height(0);
		} else {
			$(this.cssSelector.volumeBarUp).height((1-p)*h+10);
			$(this.cssSelector.volumeBarBtn).show();
			$(this.cssSelector.volumeBarBtn).css("top",(1-p)*h+25);
			if(p<0.01){
				$(this.cssSelector.volumeBarBtn).hide();
				$(this.cssSelector.volumeBarUp).height(220);
			}
		}
		
	},
	
	SeekBarFunc : function() {
		if (this.seekevent == "empty")
		{
			AudioPlayer.seekTimeout = setTimeout("AudioPlayer.SeekBarFunc()", 100);
		} else {
			AudioPlayer.seekBar(AudioPlayer.seekevent);
			AudioPlayer.seekevent = "empty";
			AudioPlayer.seekTimeout = setTimeout("AudioPlayer.SeekBarFunc()", 500);
		}
	},
	
	init:function(selector, args) {
		this.playlist = args.playlist || this.playlist;
		this.onplay = args.onplay || this.onplay;
		this.playindex = 0;
		
		var ap = this;
		$(document).ready(function(){
			ap.jPlayerSelect = selector;
			$(selector).jPlayer({
				ready: function (event) {
					$(this).jPlayer("setMedia", ap.preventCache(ap.playlist[ap.playindex]));
					ap.inited = true;
					if (ap.shouldplayAfterInit) {
						ap.play();
						ap.shouldplayAfterInit = false;
					}
				},
				swfPath: "data/js",
				supplied: "mp3,m4a,ogg,oga,wav,wave",
				//supplied: "mp3,m4a",
				volume: 1,
				wmode: "window",
				solution: (jQuery.browser.mozilla) ? "flash,html" : "html,flash",
				errorAlerts: false,
				warningAlerts: false,
				cssSelectorAncestor: "#floatingbar",
				cssSelector: ap.cssSelector,
				timeupdate: function(event) {
					var percent = parseInt(event.jPlayer.status.currentPercentAbsolute, 10),
						totalwidth = $(ap.cssSelector.seekBarBkg).width(),
						tLeft = 0;
					$(ap.cssSelector.seekBarBtn).css('left', (percent*totalwidth)/100);
					$(ap.cssSelector.seekBarLeft).width((percent*totalwidth)/100);
										
					if(event.jPlayer.status.duration > 0) {
						tLeft = event.jPlayer.status.duration - event.jPlayer.status.currentTime;
					}
					$(".timer02").text($.jPlayer.convertTime(tLeft));
				},
				noSolution: function(e) { // Handles clicks on the error message
					// Added to avoid errors using cssSelector system for no-solution
				},
				play: function(event) {
					if(typeof AudioPlayer.playlist[AudioPlayer.playindex].path != 'undefined')
					{
						var path = t4u.decodeURI(AudioPlayer.playlist[AudioPlayer.playindex].path.split('?t=')[0]);
						playlist.addRecent(path);
					}
					ap.onplay(ap.playlist[ap.playindex]);
					ap.isplaying = true;					
					ap.isPaused = false;
					$(ap.cssSelector.play).hide();
					$(ap.cssSelector.pause).show();
					var songName = ap.playlist[ap.playindex].songname + ' by ' + (typeof ap.playlist[ap.playindex].artist != 'undefined' && ap.playlist[ap.playindex].artist != 'undefined' && !$.isEmptyObject(ap.playlist[ap.playindex].artist)?ap.playlist[ap.playindex].artist:multiLang.search('Unknown artist'));
					if(songName.length > 70)
						songName = songName.substring(0, 70) + '...';
					$(ap.cssSelector.songName).text(songName);
					if(ap.playlist[ap.playindex].albumphoto == '' || ap.playlist[ap.playindex].albumphoto == null)
					{
						$(ap.cssSelector.albumBack).attr('src',getNoThumbnailBG(ap.playlist[ap.playindex].path, '33x35', 'back'));
						$(ap.cssSelector.albumPhoto).attr('src', getNoThumbnailBG(ap.playlist[ap.playindex].path, '33x35', 'image'));
						$(ap.cssSelector.albumPhoto).parent().addClass('thumbnailGenerator_front01');
						$(ap.cssSelector.albumBack).show();
					}
					else
					{
						$(ap.cssSelector.albumPhoto).parent().removeClass('thumbnailGenerator_front01');
						$(ap.cssSelector.albumBack).hide();
						$(ap.cssSelector.albumPhoto).attr('src', ap.playlist[ap.playindex].albumphoto);
					}
					$(ap.cssSelector.albumName).text(ap.playlist[ap.playindex].albumname != 'undefined' && !$.isEmptyObject(ap.playlist[ap.playindex].albumname)?ap.playlist[ap.playindex].albumname:multiLang.search('Unknown album'));
					//init scrollbar
					scrollbarInit();
				},
				pause: function(event) {
					$(ap.cssSelector.play).show();
					$(ap.cssSelector.pause).hide();
					ap.isplaying = false;					
					ap.isPaused = true;
				},
				ended: function(event) {
					if (ap.loopMode == loopModes.LoopOne)
					{
						ap.playindex--;
						ap.next();
						var jp = $(ap.jPlayerSelect);
						ap.onplay(ap.playlist[ap.playindex]);						
						jp.jPlayer("playHead", 0);
					}
					else
					{			
						ap.next();
					}
					ap.isPaused = false;
				},
				//solution: 'html, flash',
				wmode: "window"
				//,errorAlerts: true
			});
			var jp = $(ap.jPlayerSelect);
			//jp.jPlayer("option","verticalVolume", true);
			var isiPadOriPhone = navigator.userAgent.match(/iPad/i) || navigator.userAgent.match(/iPhone/i);
			jp.bind($.jPlayer.event.error + ".myProject", function(event) { // Using ".myProject" namespace
				//alert("Error Event: type = " + event.jPlayer.error.type); // The actual error code string. Eg., "e_url" for $.jPlayer.error.URL error.
				switch(event.jPlayer.error.type) {
					/*
					case $.jPlayer.error.URL:
					  reportBrokenMedia(event.jPlayer.error); // A function you might create to report the broken link to a server log.
					  getNextMedia(); // A function you might create to move on to the next media item when an error occurs.
					  break;
					case $.jPlayer.error.NO_SOLUTION:
					  // Do something
					  break;
				    */
					case $.jPlayer.error.URL:
						if(AudioPlayer.playindex == -1)
							return;
						if(!/Chrome/.test(navigator.userAgent) && !isiPadOriPhone) { 
							if(!FlashDetect.versionAtLeast(10)){ // at least v.10 flash to play music
								t4u.showMessage(multiLang.search("Your browser needs Flash plug-in to play music. Click OK to download and install"));
							}  
							else
							{
						var msg=multiLang.search('File name or path not supported');
						t4u.showMessage(msg);
							}
						}
						else
						{
							var msg=multiLang.search('File name or path not supported');
							t4u.showMessage(msg);
						}
						
						hideMusicBar();						
						break;
					case $.jPlayer.error.NO_SUPPORT:
						if(AudioPlayer.playindex == -1)
							return;
						if(!/Chrome/.test(navigator.userAgent) && !isiPadOriPhone) { 
							if(!FlashDetect.versionAtLeast(10)){ // at least v.10 flash to play music
								t4u.showMessage(multiLang.search("Your browser needs Flash plug-in to play music. Click OK to download and install"));
							}  
							else
							{
								var msg=multiLang.search('Unknown file type');
								t4u.showMessage(msg);
							}
						}
						else
						{
						var msg=multiLang.search('Unknown file type');
						t4u.showMessage(msg);
						}						
						hideMusicBar();						
						break;
				  }
				});
				
			jp.bind($.jPlayer.event.play, function() { // Bind an event handler to the instance's play event.
			  ap.onplaysuccessfully();
			});
			
			// init button status.
			$(ap.cssSelector.play).hide();
			$(ap.cssSelector.pause).show();				
			ap.setLoopBtnStatus();
			ap.setShuffleBtnStatus();
						
			// hook button functions.
			$(ap.cssSelector.prev).click(function(){ap.prev();});
			$(ap.cssSelector.next).click(function(){ap.next();});
			$(ap.cssSelector.seekBarBkg).mousedown(function(e) {
				AudioPlayer.seeking = true;
				AudioPlayer.seekevent = e;
				AudioPlayer.SeekBarFunc();
			}).mouseup(function (e) {
				AudioPlayer.seeking = false;
				clearTimeout(AudioPlayer.seekTimeout);
			}).mousemove(function (e) {
				if (AudioPlayer.seeking)
				{
					AudioPlayer.seekevent = e;
				}
			});			
			$(ap.cssSelector.loop).click(function(){ap.ToggleLoop();});
			$(ap.cssSelector.shuffleBtn).click(function(){ap.ToggleShuffle();});
			$(ap.cssSelector.volumeBarBkg).click(function(e){ap.volumeBar(e);});
			$(ap.cssSelector.musicInfo).click(function(e){ap.onmusicBarInfoClick(e);});
		});	
	},
	
	//callbacks.
	onplay:function(playsong){},
	onstop:function(playsong){},
	onplaylistchange:function(newplaylist){},
	onmusicBarInfoClick:function(e){},
	onplaysuccessfully:function(){}
};