//var __DEBUG__ = true;
//Multi_Language
var langDir='data/langpack';
var langUrl={/*'zh-tw':'lang.zh.js','zh':'lang.zh.js',*/'en-us':'lang.en.js', 'fr':'lang.fr.js', 'it':'lang.it.js', 'de':'lang.de.js', 'es':'lang.es.js'};
var langVal={/*'zh-tw':'zh','zh':'zh',*/'en-us':'en','fr':'fr','it':'it','de':'de','es':'es'};
var defaultLang='en-us';

var serverPath = './';

var virtualPath = serverPath + "data";

//XML
var xmlUrl= serverPath + 'NimbusAPI';

//upload
var uploadUrl=serverPath + 'NimbusAPI/upload?rn'; 

var timeout={short:20000,long:1800000};

// default timeout 10000ms, if upgrade firmware up to 1800000ms
var default_api_timeout = timeout.short;


//swfPath
var swfPath=virtualPath+'/js/Jplayer.swf'; //It enable player to play in IE 7 & 8.


//like sessionId
var token="";

//FolderViewer
var nowPath='';

//url of adding token
var url="";

//Photo path
var webImageUrl= serverPath + 'data/images/';

//ssid information
var wifissid;
var wifiprotocol;

//Audio default img and thumbnail count
var defaultImg=virtualPath + "/images/imgs/music_default_img.png";
var loadImg=virtualPath + "/images/loading.gif";
var needThumbnailCount=3;
var minThumbnailSize=1024;


//key number
var keyShiftNum=16;
var keyCtrlNum=17;

//Context Menu
var contextMenuStr=['Copy selected', 'Copy','Paste','Play','Add to playlist','More songs by this artist','Remove','Detail','Rename','Add to Now playing list'];

//playlist cookie
var playlistCookieName=['pl1','pl2','pl3'];
var playlistDefaultName=[('Playlist 1'),('Playlist 2'),('Playlist 3')];
var playlistRecent='plRecent';
var songs_count=20;
var cooieExpiredays=30;


//Photo Background
var photoBackColor='black';

//Audio Filter
var audioFilter=['path','album','artist','genre','title'];

//The system mode
var mode='folder';
var modeCategory={folder:'folder',photo:'photo', photoview:'photoview',video:'video',setting:'setting',recent:'recent',artist:'artist'
			,album:'album',allsong:'allsong',playlist:'playlist',genre:'genre',nowPlayingList:'nowPlayingList',search:'search'};


//System Information to users
var infoDel='Are you sure you want to delete these file(s)?';
var infoPlFull='The playlist is full.';
var infoPlWillFull='Can not add more items to this playlist';
var infoPlNameduplicated='Cannot duplicate playlist name, choose another name';
var infoPlNameNeed='Name field is required.';

//infinite-scroll info
var inifiniteScrollSize = 30;
var inifiniteScrollImgSize = 30;

var batteryRefreshDuration = 50000;

//bg & pattern index
var thumbnail_bg_count = 6;
var thumbnail_pattern_count = 8;

var SUPPORTED_FILE_FORMAT = {
		'doc': ['.doc', '.docx'],
		'xls': ['.xls', '.xlsx'],
		'ppt': ['.ppt', '.pptx'],
		'pdf': ['.pdf'],
		'txt': ['.txt']
	};
var SUPPORTED_FILE_FORMAT_COMPILED = (function(pattern){
	var compiled = [];
	var n = 0;
	var fn = function(n) {return new String(Math.floor(n / 10)) + new String(n % 10);};
	for (type in pattern) {
		var format = pattern[type];
		compiled.push(type + '/');
		for (var j = 0; j < format.length; j++)
			compiled.push('|' + format[j] + '|' + fn(n) + fn(type.length));
		n += type.length + 1 + format.join('').length + format.length * (2 + 4);
	}
	compiled.push('|');

	return compiled.join('');
})(SUPPORTED_FILE_FORMAT);

var cssRadioButtonSelected  = 'blue';
var popupBlue = {'background':'url(data/images/icons/grayRadioBtn.png) no-repeat left top'};
var popupWhite = {'background':'url(data/images/icons/whiteRadioBtn.png) no-repeat left top'}; 
