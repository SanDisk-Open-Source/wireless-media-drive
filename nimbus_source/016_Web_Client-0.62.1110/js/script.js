

function init(){
	if (jQuery('#cboxOverlay').is(':visible'))
		return;
	var space=20;
	var winW=$(window).width();
	var naviW=$("#navigator").width();
	$('#container').css('width',winW-space);	
	//$('#content').css({'width':winW-naviW-space,'margin-left':naviW});	
	$("#floatingbar").css('width',winW);
	$("#searchContainer").css({'margin-left':naviW,'background':'#ffffff','width':winW});		  
    $('#navigator').css('height',$(window).height());
      
	$("#volumeProgressBar").hide();
	$("#jquerySliderContainer").hide();
	$(".musicVolume").toggle(
		function(){
			$("#volumeProgressBar").show();
			$("#jquerySliderContainer").show();	
		},function(){
			$("#volumeProgressBar").hide();
			$("#jquerySliderContainer").hide();	
		});
}
function floatDiv(){
		$("#searchContainer").floatdiv("lefttop");
		$("#navigator").floatdiv("lefttop");
		$("#floatingbar").floatdiv("leftbottom");
	 	$("#volumeProgressBar").floatdiv("leftbottom");
}

function getBGIndex(filename) {
	var total = 0;
	for(var i = 0; i < filename.length; i++)
		total += filename.charAt(i).charCodeAt(0);
	return total % thumbnail_bg_count + 1;
}

function getPatternIndex(path) {
	var total = 0;
	for(var i = 0; i < path.length; i++)
		total += path.charAt(i).charCodeAt(0);
	return total % thumbnail_pattern_count + 1;
}
function getNoThumbnailBG(path, size, type) {
	if(path == null || typeof path === 'undefined' || typeof path === 'object')
		path = "";
	else if(typeof path == 'number')
		path = "" + path;
	var filename = path.replace(/^.*[\\\/]/, '');
	var bg_index = getBGIndex(filename);
	var pattern_index = getBGIndex(path);
	size = typeof size != 'undefinded'?size:'144x144'; // avaiable size are "38x47", "145x110", "144x144", default 144x144
	if(!type) {
	var width = size.substr(0, size.indexOf('x'));
	var height = size.substr(size.indexOf('x') + 1);	
	var cssStyle="";
	if(size=="144x144"){
		cssStyle="left:-2px; top:-2px;";	
		}
		else
		 cssStyle= size=="38x47"?' display: inline-block; margin:7px 20px -7px 6px;':'';
		 width=Number(width)+1;
	var imgWH="width='" + width + "' height='" + height + "'";	
	var html = '' +
	'<div class="photoThumbnailGeneratorMargin" style="width: ' + width + 'px; height: ' + height + 'px;'+cssStyle+'">' + 
		'<div class="size_'+ size +'  thumbnailGenerator_background0'+ bg_index +'"><img border="0" src="'+virtualPath+'/images/pattern/pattern0'+ bg_index +'.jpg" '+ imgWH +' /></div>' +
		 '<div class="size_'+ size +'  thumbnailGenerator_front0'+ pattern_index +'"><img border="0" src="'+virtualPath+'/images/gradient/gradient0'+ bg_index +'.jpg"  '+ imgWH +' /></div>' +
	'</div>';
	} else if(type == 'image') {
	    var html = './data/images/gradient/gradient0'+ bg_index +'.jpg';
	} else {
	    var html = './data/images/pattern/pattern0'+ bg_index +'.jpg';
	}
	return html;
}

function setTableWidthByTable(gridViewData, gridViewTitle)
{	
	var isReady = gridViewData.find('tr:visible').length > 0;
	scrollbarInit();
	var len = $('tr:visible:eq(0) > td' ).length;
	gridViewData.find('tr:visible:eq(0) > td').each(function(i, e){
		if (i === len - 1 ) 
		{
		gridViewTitle.find('tr:visible:eq(0) > td').eq(i).width('35px');//Andrew
		}
		else
		{			
			gridViewTitle.find('tr:visible:eq(0) > td').eq(i).width($(this).width());
		}
	});
}

function setFixedTitle() {
	var index = 0;
	if(mode == modeCategory.playlist && audioView.moreSongByThisArtistInPlaylist != true)
		index = 1;
	selector = '.fixedTitleHead table';
	$(selector).eq(index).width($('#content').width() - 100);	
	
	$(selector).eq(index).find('tr:visible:eq(0)>td').each(function(i) {
	  $('.musicListTitleHead .musicListTitle').eq(index).find('div').eq(i).width($(this).width());
	});	
}

function setFixedTitleNoData() {
	var index = 0;
	if(mode == modeCategory.playlist && audioView.moreSongByThisArtistInPlaylist != true)
		index = 1;
	selector = '.fixedTitleHead table';
	$(selector).eq(index).width($('#content').width() - 100);	
	
	$(selector).eq(index).find('tr:eq(0)>td').each(function(i) {
	  $('.musicListTitleHead .musicListTitle').eq(index).find('div').eq(i).width(($(this).width() /100)*$(this).parent().parent().width());
	});	
}

function scrollbarInit(){
	if (jQuery('#cboxOverlay').is(':visible'))
		return;
	var space=1,
		winW=$(window).width(),
		winH=$(window).height(),
		naviW=$("#navigator").width(),
		searchH=$("#searchContainer").height(),
		barH=$("#floatingbar").height(),
		pos='fixed',
		cssStyle="",
		musicScrollbarHeight = winH-searchH-147,
		folderViewHeight = winH-searchH-40;
	
	if(winW>=855){
		$('#divFolderView table').attr('width',winW-naviW-space-35-35);	
		$('#dropBoxDivTitle').attr('width','100%');
	}
	else{
		//$('#divFolderView table').attr('width',560);
		//$('#dropBoxDivTitle').attr('width',560);
		pos='relative';
	}
	$('#divFolderView .nameData').width($('#dropBoxDivTitle tr td').eq(1).width() -30);
	if(AudioPlayer.isplaying || AudioPlayer.isPaused)
		folderViewHeight = folderViewHeight - barH;
	$('#divFolderView').attr('style','width:'+(winW-naviW-space-35)+'px;height:'+(folderViewHeight)+'px;overflow-x:hidden;overflow-y:auto;position:'+pos);
	
	if ($.browser.msie  && parseInt($.browser.version, 10) === 8){
	  cssStyle=" margin-left: 30px;";	  
	}
	
	
	if($('#divMusicViewerList').css('display')=='block' && $('.musicListInfo').css('display')=='block' )		
		musicScrollbarHeight = musicScrollbarHeight - barH;
	if(mode == modeCategory.genre || mode == modeCategory.artist || mode == modeCategory.album)
		musicScrollbarHeight = musicScrollbarHeight - 95;
	$('#divMusicViewerList-List').attr('style','width:'+(winW-naviW-space-35)+'px;height:'+musicScrollbarHeight+'px;overflow-x:hidden;overflow-y:auto;'+cssStyle);				
	
	if($('#divMusicViewerPlList').hasClass("playlist"))	
		$('#divMusicViewerPlList').attr('style','width:'+(winW-naviW-space-35)+'px;height:'+(winH-searchH-barH-130)+'px;overflow-x:hidden;overflow-y:auto');
}

function reorder_playlist() {
    var _cookie_names = ['pl1','pl2','pl3'];
    var _names = {};
    var sort = [];
    var pls = {};
    for(var k in _cookie_names) {
	var cookie= _cookie_names[k];
	var pl=$.secureEvalJSON(t4u.properties(cookie));
	pls[pl.name] = pl;
	_names[pl.name] = $('tr[cookiename="'+ cookie +'"]');
	sort.push(pl.name);
    }
    sort.sort();

    $('.musicList>table>tbody>tr').not(":first-child").remove();

    for(var i = 0; i < sort.length; i++) {
	var key = sort[i];
	var o = $(_names[key][0].outerHTML);
	o.find('td:eq(1)').text(i+1);
	$('.musicList>table>tbody').append(o);
	pls[key].name = key;
	t4u.properties(_cookie_names[i],JSON.stringify(pls[key]), { expires: cooieExpiredays });
    }
}


$(function(){					
	/*
	init();
	
	$(window).resize(function() {							  
			init();	
	});
	floatDiv();
	*/
});  
 