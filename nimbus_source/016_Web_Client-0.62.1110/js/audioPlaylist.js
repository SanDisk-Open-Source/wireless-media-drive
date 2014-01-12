;(function($){
	
	/*** 
		Author:Tim
		Desc: To analyse the JSON and to play this song in the audio player.Three step as the following.Firt,generate data for the audio player.Second,generate the unikey which we use the file path now.Last,to play the songs.
		@param {object} dataJson - API returns.
		@return nothing
	*/
	function analyseData(dataJson){
	
		if(typeof dataJson.nimbus!='undefined'){
			dataJson=getObjEntry(dataJson);
		}
	
		if(typeof dataJson.path != 'undefined')
		{
			var musicFormat=dataJson.path.split('.');
			musicFormat=""+musicFormat[musicFormat.length-1];
			
			var result={albumname:dataJson.album,songname:dataJson.title,albumphoto:'',artist:dataJson.artist, path:dataJson.path};
			
			$('.albumback').hide();
			
			if(typeof dataJson.thumbnail == 'object')
				dataJson.thumbnail = "";
			judgeThumbnail(dataJson.thumbnail,result);
			
			result[musicFormat]=dataJson.path;
			
			result['path']=dataJson.path;
			
			var uniKey=(dataJson.path);
			audioPlaylist.play(result,uniKey);	
		}
	
	}
	
	
	var audioPlaylist = {
		list:[],	
		inited:false,
		playlistSongCount:0,//存放有幾首歌
		prependList: [],
		
		/*** 
			Author:Tim
			Desc: Create the default cookies.
			@param nothing
			@return nothing
		*/
		init:function(){
			
			var i=0;			
			while(playlist.create({name:escape(playlistDefaultName[i]),songs:[]})){
				i++;			
			}
			
		},
		
		/*** 
			Author:Tim
			Desc: To play the song.If the unikey is undefined,we will generate it.
			@param {object} dataJson - API returns.
			@param {string} uniKey - cookie will save it, in order to make the recent playlist.
			@return nothing
		*/
		play:function(dataJson,uniKey){		
			
			
			if(typeof uniKey=='undefined'){
				analyseData(dataJson);
				return;
			}
			
			AudioPlayer.playlist.push(dataJson );
			if(!audioPlaylist.inited){
				audioPlaylist.init(); //in order to initial the Audio Player and create the playlist.(the init function need the playlist which contain song(s))
				AudioPlayer.init('#audioComponents',AudioPlayer.playlist);
				audioPlaylist.inited=true;
			}
			
			AudioPlayer.onplaysuccessfully = function()
			{
				showMusicBar();
			}			
			
			if(!AudioPlayer.isplaying){
				 AudioPlayer.play();
			}
			
			window.setTimeout(function() {
				AudioPlayer.playindex=-1; AudioPlayer.next();
			},50);
			//In order to immediately play the song which user selects ,we need to set the playindex as -1 and call next.(Do that,because of stopping the audio player is asynchronous.)
		},
		/*** 
			Author:Tim
			Desc: To play the song.If the unikey is undefined,we will generate it.
			@param {object} dataJson - API returns.
			@param {string} uniKey - cookie will save it, in order to make the recent playlist.
			@return nothing
		*/
		playInFolder:function(dataJson,uniKey){		
			
			if(typeof dataJson.path != 'undefined')
			{
				var musicFormat=dataJson.path.split('.');
				musicFormat=""+musicFormat[musicFormat.length-1];
				
				var result={albumname:dataJson.album,songname:dataJson.title,albumphoto:'',artist:dataJson.artist, path:dataJson.path};
		
				$('.albumback').hide();
				
				if(typeof dataJson.thumbnail == 'object')
					dataJson.thumbnail = "";
				judgeThumbnail(dataJson.thumbnail,result);
				
				result[musicFormat]=t4u.encodeURI(dataJson.path);
				
				result['path']=dataJson.path;
			}
			dataJson = result;
			
			AudioPlayer.playlist.push(dataJson );			
		},
		/*** 
			Author:Tim
			Desc: To clear the AudioPlayer.playlis and to stop the AudioPlayer.
			@param nothing
			@return nothing
		*/
		clearAndStop:function(){
			AudioPlayer.playindex=-1;  
			AudioPlayer.stop();
			AudioPlayer.playlist=[];
			
		},
		
		/*** 
			Author:Tim
			Desc: To play a song of playlist.
			@param {int} plNum - the number of playlist(cookie), according to the define.js.
			@param {int} index - the index of the song in this playlist.
			@return nothing
		*/
		playPlSong:function(plNum,index){
		
			var uniKey = playlist.getSong(plNum,index);
			var filter=unescape(uniKey);
			t4u.bgProcess = true;
			t4u.nimbusApi.fileExplorer.getMediaList(function(data){
					if(t4u.nimbusApi.getErrorCode(data) == 0)
					{
						if(AudioPlayer.playlist.length == audioPlaylist.playlistSongCount)
							return;
						//file does not exist
						if(typeof data.nimbus.items.entry == 'undefined')
						{
							playlist.removeSong(filter);
							return;
						}
						analyseData(data);
					}
					else
					{
						var msg = getObjData(data.nimbus.errmsg);
						t4u.showMessage(msg);
					}
				},
				function(data){
				},				
				{'mediaType':[{'type':'music'}],'sort':[{'order':'asc','field':'title'}]},
				{filter:[{'field':'path','value':filter}]}
			);
		},
		
		/*** 
			Author:Tim
			Desc: To play all songs of playlist.
			@param {int} num - the number of playlist(cookie), according to the define.js.
			@return nothing
		*/
		playAllPlayListSongs:function(num){
		
			audioPlaylist.clearAndStop();
			
			var pl= playlist.get(num);
			
			audioPlaylist.playlistSongCount = pl.songs.length;
			for(var i=0;i<pl.songs.length;i++){
				audioPlaylist.playPlSong(num,i);				
			}
		},
		
		/*** 
			Author:Steven
			Desc: Stop and Play, modifing Now Playing List behavior
			@param {int} index - the index of the songs in Now Playing list
			@return nothing
		*/
		stopAndPlay:function(index){
			AudioPlayer.playindex=-1;  
			AudioPlayer.stop();
			AudioPlayer.playindex=index;

			if(!audioPlaylist.inited){
				audioPlaylist.init(); //in order to initial the Audio Player and create the playlist.(the init function need the playlist which contain song(s))
				AudioPlayer.init('#audioComponents',AudioPlayer.playlist);
				audioPlaylist.inited=true;
			}
			
			AudioPlayer.onplaysuccessfully = function()
			{
				showMusicBar();
			}			
			
			if(!AudioPlayer.isplaying){
				 AudioPlayer.play();
			}
			
			window.setTimeout(function() {
				AudioPlayer.playindex=index - 1; AudioPlayer.next();
			},50);
		}
	};
	// audioPlaylist.init();
	window.audioPlaylist = audioPlaylist;
})(jQuery);