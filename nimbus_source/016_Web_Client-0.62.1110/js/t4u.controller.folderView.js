;(function($){

	/*** 
		Author:Tim
		Desc: All selector strings in folder view.
	*/
	var cssSelector={
		linkPath:'.pathLink',
		linkSort:'.sort',
		btnDel:'#btnDel',
		btnSearch:'.searchBtn',
		listFolder:'#divFolderView table tr:not(":first-child")',
		listFolder_click:'#divFolderView table tr:not(":first-child") td:not(":first-child")',
		checkbox_click: '#divFolderView table tr td input[type=checkbox]',
		panelPath:'.path',
		panelListFolder:'#divFolderView table tr:visible',
		header:'#dropBoxDivTitle tr td'
	};
	
	
	/*** 
		Author:Tim
		Desc: To bind the mouseenter, mouseleave, dblclick, click event.
		@param nothing
		@return nothing
	*/
	function folderViewerController()
	{	
		$(document).on('mouseenter',cssSelector.listFolder, mouseover);
		$(document).on('selected',cssSelector.listFolder, selected);
		$(document).on('mouseleave',cssSelector.listFolder, mouseout);
		$(document).on('click',cssSelector.listFolder_click, dblclick);
		$(document).on('click',cssSelector.checkbox_click, click);
		
		$(document).on('mouseenter', cssSelector.header, headerMouseOver);			
		$(document).on('mouseleave', cssSelector.header, headerMouseOut);
	}
	
	/*** 
		Author:Tim
		Desc: To bind the click event of path link.
		@param nothing
		@return nothing
	*/
	function pathLinkController(){
	
		$(document).on('click',cssSelector.linkPath,pathLinkClick);
	}
	
	/*** 
		Author:Tim
		Desc: To bind the click event of sort button.
		@param nothing
		@return nothing
	*/
	function btnSortController(){
		
		$(document).on('click',cssSelector.linkSort,btnSortClick);
	}
	
	/*** 
		Author:Tim
		Desc: To process non-dir file.
		@param {object} selector - the non-dir file.
		@return nothing
	*/
	function processNonDirFile(selector, event){
		if (!$(event.target).is('A')) {
			var obj = selector.find('a');
			obj.trigger('click');
				}
	}
	
	/*** 
		Author:Tim
		Desc:Changing the nowPath and showing something below the now path.
		@param {string} path - the whole file path.
		@return nothing
	*/
	function goTo(path){
	
		$(cssSelector.panelPath).html(setPathLink(path));
		nowPath=path;
		
	}
	
	/*** 
		Author:Tim
		Desc:the function will generate the path of the folderview.When user is in the root,he will look the 'home'.
		@param {string} path - the whole file path.
		@return {string} - the whole file path which contains hyperlink(s).
	*/
	function setPathLink(path){
	
		var root= t4u.format('<a class="pathLink" href="javascript:void(0);">{0}</a>','Home');
	
		if(path=='')
			return root;
	
		var str='';
		var pathArr=path.split('/');
		for(var i=0;i<pathArr.length;i++){
			if(pathArr[i]=='') continue;		
			var tag='';
			for(var j=0;j<=i;j++){
				tag=tag+'/'+pathArr[j];
				tag=tag.replace(/\/+/mg,'/');
			}
			str+=t4u.format('/<a class="pathLink" tag="{0}" href="javascript:void(0);">{1}</a>',tag,pathArr[i]);
		}
	
		return root+str;
	}
	
	/*** 
		Author:Tim
		Desc:the function will sort the data in the folder view.
		@param nothing
		@return nothing
	*/
	function btnSortClick(){
		var asc,
			sortButtons = $(cssSelector.linkSort);
		sortButtons.hide();	
		sortButtons.attr('tag', '');		
		if($(this).css('background-image').indexOf('Down')>-1){
			asc=true;
			$(this).css('background-image','url(/data/images/icons/grayArrowUp.png)') ;
		}else{
			asc=false;
			$(this).css('background-image','url(/data/images/icons/grayArrowDown.png)');
		}
		var sortIndex = $(this).index(cssSelector.linkSort);
		sortButtons.eq(sortIndex).show();
		sortButtons.eq(sortIndex).attr('tag', 'selected');		
		t4u.blockUI();
		setTimeout(function(){
			switch(sortIndex)
			{
				case 0:
					folderViewer.sort('name',asc);
					break;
				case 1:
					folderViewer.sort('modifiedUTC',asc);
					break;
				case 2:
					folderViewer.sort('format',asc);
					break;
				case 3:
					folderViewer.sort('bytes',asc);
					break;
				default:
					t4u.unblockUI();
					return;
			}
			t4u.unblockUI();
		}, 100);
		
	}
	
	/*** 
		Author:Tim
		Desc: (bl) Showing the info by this.attr('tag') 
		@param nothing
		@return nothing
	*/
	function pathLinkClick(){
	
		$(cssSelector.listFolder).remove();
		
		if($(this).text()=='Home'){
			
			nowPath='/';
			folderViewer.counter=1;
			folderViewer.isAllData=false;
			folderViewer.setup(nowPath);  //if users want to back the root,we will callback again. 
			return;
		}
		
		nowPath=$(this).attr('tag');
		folderViewer.counter=1;
		folderViewer.isAllData=false;
		folderViewer.isAppend=false;
		folderViewer.setup(nowPath);
		
	}
	
	/*** 
		Author:Tim
		Desc: (bl) mouseover
		@param nothing
		@return nothing
	*/
	function mouseover(){	
		$(this).find('.thumbnailShowRighClickArrow').show();
		if($(this).attr('tag') != 'clicked')
			$(this).css({'background':'#e9e9e9'});			
	}
	
	function headerMouseOver(){	
		$(this).css({'background-repeat':'repeat-x'});	
		$(this).find('a').show();
	}
	
	function headerMouseOut(){	
		$(this).css({'background-repeat':'no-repeat'});					
		if($(this).find('a').attr('tag') != 'selected')
			$(this).find('a').hide();
	}
	
	function selected(){
		$(this).css({'background':'#33b5e5'});
	}
	
	/*** 
		Author:Tim
		Desc: (bl) mouseout
		@param nothing
		@return nothing
	*/
	function mouseout(){			
			if($(this).attr('tag')=='clicked') return;
			$(this).css({'background':''});
			$(this).find('.thumbnailShowRighClickArrow').hide();
	}
	
	/*** 
		Author:Tim
		Desc: (bl) click
		@param nothing
		@return nothing
	*/
	function click(){			
	
		eventClick(cssSelector.listFolder,cssSelector.panelListFolder,$(this.parentNode.parentNode));
		
	}
	
	/*** 
		Author:Tim
		Desc: (bl) dblclick. If this is a dir, we will go to this path. If this is not a dir, we will process it.
		@param nothing
		@return nothing
	*/
	function dblclick(event){	
		var $tr = $(this.parentNode);
		if($tr.attr('isdir')=='false'){  //if the item which is clicked is not a dir, we will ignored.
			processNonDirFile($tr, event);
			return;
		}
			
		$(cssSelector.listFolder).remove();
		var nextPath=$tr.attr('nextPath');	
		nowPath=nextPath;
		folderViewer.counter=1;
		folderViewer.isAllData=false;
		folderViewer.isAppend=false;
		folderViewer.setup(nextPath);
	}
	
	var controllerFolder = {
		
		/*** 
			Author:Tim
			Desc: setup all controller
			@param nothing
			@return nothing
		*/
		setup: function(){
					
			folderViewerController();
			btnSortController();
			pathLinkController();
			
		}
	};
	
	controllerFolder.setup();
	window.controller.folderView = controllerFolder;
	
})(jQuery);
