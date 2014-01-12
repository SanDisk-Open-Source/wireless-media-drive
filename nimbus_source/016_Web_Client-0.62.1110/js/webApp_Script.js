
var showToolbar = true;
function showScrollbar(show)
{
	if ($.browser.msie  && parseInt($.browser.version, 10) === 7){
	showToolbar = show;
	init();
	}	
}
function init(){
	if (jQuery('#cboxOverlay').is(':visible'))
		return;
	var space=1,	
		winW=$(window).width(),
		winH=$(window).height(),
		naviW=$("#navigator").width(),
		searchH=$("#searchContainer").height(),
		barH=$("#floatingbar").height(),
		height = winH-searchH-100,
		overflowY = 'hidden',
		overflowX = 'hidden',
		contentW = winW-naviW-space;	

		if(AudioPlayer.isplaying || AudioPlayer.isPaused || mode == modeCategory.artist || mode == modeCategory.album || mode == modeCategory.genre)
			height = height - barH;
		else if(mode == modeCategory.setting)
			height = height+30;
	if(showToolbar)
		overflowY = 'auto'
	else
		overflowY = 'hidden';
	if(winW>=855)	
		overflowX = 'hidden';
	else{
		overflowX = 'auto';	
	}
	
        $('#content').css({'width':winW-naviW-space,'height':height+'px','margin-left':naviW,'overflow-x':'hidden','overflow-y':overflowY});	
	$("#floatingbar").css('width',winW);
	$("#searchContainer").css({'width':winW});		  
    $('#navigator').css({'height':$(window).height(),'margin-Top':'51px'});
	$('#jquerySlider').css({'left':winW-100});
	$("#jquerySliderContainer").hide();		
	
	//windowClick();	
	musicVolumeToggle();
	$(document).bind('click',volumeProgressBarHandler);
	$('#volumeProgressBar div,#volumeProgressRateBar').mouseover(function(){$(document).unbind('click',volumeProgressBarHandler)});
	$('#volumeProgressBar').mouseout(function(){$(document).bind('click',volumeProgressBarHandler)});
	
	setFixedTitle();
	$('.blockUI.blockMsg').not('.blockElement').center();
	if(mode == modeCategory.folder && $('.entryTr').is(':visible'))
	{
		folderViewer.resetData();
	}
}

function windowClick(){
$(document).click(function(){
		
		volumeProgressBarHandler();
	});	

}
function volumeProgressBarHandler(){
	try
	{
	if($('#jquerySliderContainer').css('display') == 'block') {
			$( "#jquerySliderContainer" ).hide();
			musicVolumeToggle();
		}
}
	catch(e)
	{
	}
}
function musicVolumeToggle(){
$(".musicVolume").toggle(
	function(){
		$("#jquerySliderContainer").show();
	},function(){
		$("#jquerySliderContainer").hide();
	});
}
function floatDiv(){	
		$("#contentBg").css({'width':$(window).width()}).floatdiv("leftbottom");
		$("#searchContainer").floatdiv( "lefttop");
		$("#navigator").floatdiv("lefttop");
		$("#floatingbar").floatdiv("leftbottom");
	
		//z-index
		$("#contentBg").css({'z-index':0,'position':'relative'});
		$('#content').css({'z-index':1,'position':'relative'});
		$("#navigator").css({'z-index':3,'position':'relative'});	
		$("#floatingbar").css({'z-index':4,'position':'relative'});			
		$("#searchContainer").css({'z-index':2,'position':'relative'});
		
}
function musicListHover(){
	$(".musicList tr").not(":first-child").hover(
		function(){
			$(this).css({'background':'#33b5e5'})
		},function(){
			$(this).css({'background':'#ffffff'})
	});
}
function settingListHover(){
	$(".settingDiv").hover(
		function(){
			$(this).css({'background':'#33b5e5'})
		},function(){
			$(this).css({'background':'#ffffff'})
	});
}
function dropBoxSetting(){
	$(".dropBoxDiv tr").not(":first-child").hover(
		function(){
			$(this).css({'background':'#33b5e5'})
		},function(){
			$(this).css({'background':'none'})
	});
	$(".dropBoxDiv tr:first-child").children().css({'height':'35px','vertical-align':'top'});
	
}
function selectDropDownSelect(){	
	$(".selectDropdown").toggle(function(){									
		$(".selectDropdown ul").css({"display": "block"});
										 },
	function(){
		$(".selectDropdown ul").css({"display": "none"});
	});
	$(".selectDropdown").parent('li').css({'z-index':'9999','position':'relative'});
	
}
function sortToggle(){
	$('.sort').toggle(function(){
		$(this).css({'background':'url(images/icons/grayArrowUp.png) no-repeat 0px 5px'});
							   },function(){
		$(this).css({'background':'url(images/icons/grayArrowDown.png) no-repeat 0px 5px'});						   
							   })
	};
function photoRotateImg(){
	$(".photoAlbumsListItems .rotate01").rotate(0);
	$(".photoAlbumsListItems .rotate02").css({ 'opacity': 0.5 }).rotate(3);
	$(".photoAlbumsListItems .rotate03").css({ 'opacity': 0.5 }).rotate(-3);
}
function musicRotateImg(){
	$(".musicAlbumsListItems .rotateMusic01").rotate(0);
	$(".musicAlbumsListItems .rotateMusic02").css({ 'opacity': 0.5 }).rotate(6);
	$(".musicAlbumsListItems .rotateMusic03").css({ 'opacity': 0.5 }).rotate(-6);
}
function shwoViewport(){
       var vw=$('.popupMiddle .wifi .viewport');
       if(vw.width()<='200px')
       {
               $('.scrollbar').css({'display':'none'});
       }
}
function rightClickMennu(){
	// Show menu when #myDiv is clicked
	$(".musicList").contextMenu({
		menu: 'myMenu'
	},
		function(action, el, pos) {
		alert(
			'Action: ' + action + '\n\n' +
			'Element ID: ' + $(el).attr('id') + '\n\n' + 
			'X: ' + pos.x + '  Y: ' + pos.y + ' (relative to element)\n\n' + 
			'X: ' + pos.docX + '  Y: ' + pos.docY+ ' (relative to document)'
			);
		
	});
	
	
	// Disable menus
	$("#disableMenus").click( function() {
		$('.musicList').disableContextMenu();
		$(this).attr('disabled', true);
		$("#enableMenus").attr('disabled', false);
	});
	
}
function shwoViewport(){
	var vw=$('.popupMiddle .wifi .viewport');
	if(vw.width()<='200px')
	{
		$('.scrollbar').css({'display':'none'});
	}

	}
	
var timeOut = null;

window.onresize = function(){
    if (timeOut != null)
        clearTimeout(timeOut);

    timeOut = setTimeout(function(){
        init();
		scrollbarInit();		
    }, 500);
};	
	
var resizeTimer;
function resizeColorBox()
{
    if (resizeTimer) clearTimeout(resizeTimer);
    resizeTimer = setTimeout(function() {
            if (jQuery('#cboxOverlay').is(':visible')) {
                    jQuery.colorbox.load(true);
            }
    }, 300)
}
	
$(function(){		
	$('.wifi').tinyscrollbar({sizethumb:100 });
    	shwoViewport();		
	floatDiv();
	musicListHover();
	dropBoxSetting();	
	sortToggle();
	selectDropDownSelect();
	init();
	
	jQuery(window).resize(resizeColorBox);
});  
