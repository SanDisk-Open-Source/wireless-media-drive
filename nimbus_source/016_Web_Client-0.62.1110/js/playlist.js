;(function($){
	
	var playlist = {	
	
		/*** 
			Author:Tim
			Desc: Set the recent playlist.
			@example: var dataJson={songs:['song_unique1','song_unique2','song_unique3']};
			@param {object} dataJson - the all recent data.
			@return nothing.
		*/
		setRecent:function(dataJson){
			t4u.properties(playlistRecent,JSON.stringify(dataJson), { expires: cooieExpiredays });
		},
		
		/*** 
			Author:Tim
			Desc: Get the recent playlist.
			@example: console.log(playlist.getRecent()) //{songs:['song_unique1','song_unique2','song_unique3']};
			@param nothing.
			@return {object} dataJson - the all recent data.
		*/
		getRecent:function(){
		
			if($.secureEvalJSON(t4u.properties(playlistRecent))==null)
				playlist.setRecent({songs:[]});
			return  $.secureEvalJSON(t4u.properties(playlistRecent));
		},
		
		/*** 
			Author:Tim
			Desc: To implement that add data to the reccent playlist.The new song will be the first.If it has the same song,it need to remove the old song.If it is more than contraint of system,it will be remove old songs,too.
			@param {string} songUnikey - the unikey of the song.
			@return nothing
		*/
		addRecent:function(songUnikey){
		
			var recent=playlist.getRecent();			
			var dataJson={songs:[songUnikey]}; 
						
			for(var i=0;i<recent.songs.length;i++){
				if(recent.songs[i]==songUnikey){   //avoid the repeat song.
					continue;
				}
				dataJson.songs.push(recent.songs[i]);
			}
			
			if(dataJson.songs.length>songs_count){
				for(var j=0;j<dataJson.songs.length-songs_count;j++){
					dataJson.songs.pop();
				}
			}
			
			playlist.setRecent(dataJson);
		},		
		
		/*** 
			Author:Tim
			Desc: To create the song playlist.
			@example: var dataJson={name:'happy',songs:['song_unique1','song_unique2','song_unique3']};
			@param {object} dataJson -  The info of this playlist.
			@return {bool} - If the all cookie is created,the function will return false.
		*/
		create:function(dataJson){	 
		
			var nowPlNum=-1;
			for(var i=0;i<playlistCookieName.length;i++){
				if(t4u.properties(playlistCookieName[i])==null){
					nowPlNum=i;
					break;
				}
			}
			
			if(nowPlNum==-1) return false;
			
			while(dataJson.songs.length>songs_count){
				dataJson.songs.pop();
			}
			
			t4u.properties(playlistCookieName[nowPlNum],JSON.stringify(dataJson), { expires: cooieExpiredays });
			
			return true;
		},
		
		/*** 
			Author:Tim
			Desc: To update the playlist.
			@example: var dataJson={oriName:'happy',newName:'fun',songs:['song_unique1','song_unique3']}; //remove song_unique2 and rename list name.
			@param {object} dataJson - The info of this playlist.
			@return {bool} - If we can not search the playlist which user assigns, we will return false.
		*/
		update:function(dataJson){  
			
			
			var nowPlNum=-1;
			for(var i=0;i<playlistCookieName.length;i++){
				if(t4u.properties(playlistCookieName[i])!=null && $.secureEvalJSON(t4u.properties(playlistCookieName[i])).name==dataJson.oriName){
					nowPlNum=i;
					break;
				}
			}
			
			if(nowPlNum==-1) return false;	
				
			
			var newDataJson={name:dataJson.newName,songs:dataJson.songs};
			t4u.properties(playlistCookieName[nowPlNum],JSON.stringify(newDataJson), { expires: cooieExpiredays });
			
			return true;
			
		},
		
		/*** 
			Author:Tim
			Desc: To clear the playlist.
			@param {string} plName - The name of this playlist.
			@return {bool} - If we can not search the playlist which user assigns, we will return false.
		*/
		clear:function(plName){
		
			return playlist.update({oriName:plName,newName:plName,songs:[]})
		},
		
		/*** 
			Author:Tim
			Desc: To get the playlist by the playlist(cookie) number.
			@param {int} num - The number of this playlist(cookie).
			@return {bool} - If we do not have this, we will return false.
		*/
		get:function(num){
		
			if(num>=playlistCookieName.length)
				return false;
		
			return $.secureEvalJSON(t4u.properties(playlistCookieName[num]));					
		},
		
		/*** 
			Author:Tim
			Desc: To get a song in this playlist.
			@param {int} num - The number of this playlist(cookie).
			@param {int} index - The index of the song.
			@return {bool} - If we do not have this, we will return false.
		*/
		getSong:function(num,index){
		
			return playlist.get(num).songs[index];
		},
		
		/*** 
			Author:Tim
			Desc: To remove song in the playllist, in order to let the playlist won't have the songs which have deleted.
			@param {num} The number of this playlist(cookie).
			@param {index} The index of the song.
			@return {bool} - If we do not have this, we will return false.
		*/
		removeSong:function(songPathStr){
			
			for(var i=0;i<playlistCookieName.length;i++){
			
				var oriPl=this.get(i);
				var dataJson={oriName:oriPl.name,newName:oriPl.name,songs:[]};
				
				for(var j=0;j<oriPl.songs.length;j++){
					
					if(oriPl.songs[j]==songPathStr)
						continue;
					dataJson.songs.push(oriPl.songs[j]);	
				}
				playlist.update(dataJson);
			}
			
			var recent=this.getRecent();
			var dataJson={songs:[]};
			for(var j=0;j<recent.songs.length;j++){
					
				if(recent.songs[j]==songPathStr)
					continue;
				dataJson.songs.push(recent.songs[j]);	
			}
			this.setRecent(dataJson);
			$('#MusicViewrCount-count').text($('#divMusicViewerPlList tr').length - 1);
		
		},
		
		/*** 
			Author:Tim
			Desc: To delete the full cookie.
			@param {string} name - The name of this playlist.
			@return {bool} - If we do not have this, we will return false.
		*/
		del:function(name){
		
			name=escape(name);
		
			var nowPlNum=-1;
			for(var i=0;i<playlistCookieName.length;i++){
				if($.secureEvalJSON(t4u.properties(playlistCookieName[i])).name==name){
					nowPlNum=i;
					break;
				}
			}
			
			if(nowPlNum==-1) return false;	
			
			t4u.properties(playlistCookieName[nowPlNum],null);
			
			return true;					
		}
		
		
		
	};
	
	window.playlist = playlist;
})(jQuery);