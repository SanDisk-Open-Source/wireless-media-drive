/***任意位置浮动固定层*/
/***没剑(http://regedit.cnblogs.com) 2009-03-04*/
/***说明：可以让指定的层浮动到网页上的任何位置，当滚动条滚动时它会保持在当前位置不变，不会产生闪动*/
/***2009-06-10修改：重新修改插件实现固定浮动层的方式，使用一个大固定层来定位
/***2009-07-16修改：修正IE6下无法固定在top上的问题
/***09-11-5修改：当自定义层的绝对位置时，加上top为空值时的判断
这次的方法偷自天涯新版页
经多次测试，基本上没bug~
有问题的朋友欢迎到偶的博客http://regedit.cnblogs.com上提出
*/
/***调用：
1 无参数调用：默认浮动在右下角
$("#id").floatdiv();

2 内置固定位置浮动
//右下角
$("#id").floatdiv("rightbottom");
//左下角
$("#id").floatdiv("leftbottom");
//右下角
$("#id").floatdiv("rightbottom");
//左上角
$("#id").floatdiv("lefttop");
//右上角
$("#id").floatdiv("righttop");
//居中
$("#id").floatdiv("middle");

另外新添加了四个新的固定位置方法

middletop（居中置顶）、middlebottom（居中置低）、leftmiddle、rightmiddle

3 自定义位置浮动
$("#id").floatdiv({left:"10px",top:"10px"});
以上参数，设置浮动层在left 10个像素,top 10个像素的位置
*/
jQuery.fn.floatdiv=function(location){
		//判断浏览器版本
	var isIE6=false;
	var Sys = {};
    var ua = navigator.userAgent.toLowerCase();
    var s;
    (s = ua.match(/msie ([\d.]+)/)) ? Sys.ie = s[1] : 0;
	if(Sys.ie && Sys.ie=="6.0"){
		isIE6=true;
	}
	var windowWidth,windowHeight;//窗口的高和宽
	//取得窗口的高和宽
	if (self.innerHeight) {
		windowWidth=self.innerWidth;
		windowHeight=self.innerHeight;
	}else if (document.documentElement&&document.documentElement.clientHeight) {
		windowWidth=document.documentElement.clientWidth;
		windowHeight=document.documentElement.clientHeight;
	} else if (document.body) {
		windowWidth=document.body.clientWidth;
		windowHeight=document.body.clientHeight;
	}
	return this.each(function(){
		var loc;//层的绝对定位位置
		var wrap=$("<div></div>");
		var top=-1;
		if(location==undefined || location.constructor == String){
			switch(location){
				case("rightbottom")://右下角
					loc={right:"0px",bottom:"0px"};
					break;
				case("leftbottom")://左下角
					loc={left:"0px",bottom:"0px"};
					break;	
				case("lefttop")://左上角
					loc={left:"0px",top:"0px"};
					top=0;
					break;
				case("righttop")://右上角
					loc={right:"0px",top:"0px"};
					top=0;
					break;
				case("middletop")://居中置顶
					loc={left:windowWidth/2-$(this).width()/2+"px",top:"0px"};
					top=0;
					break;
				case("middlebottom")://居中置低
					loc={left:windowWidth/2-$(this).width()/2+"px",bottom:"0px"};
					break;
				case("leftmiddle")://左边居中
					loc={left:"0px",top:windowHeight/2-$(this).height()/2+"px"};
					top=windowHeight/2-$(this).height()/2;
					break;
				case("rightmiddle")://右边居中
					loc={right:"0px",top:windowHeight/2-$(this).height()/2+"px"};
					top=windowHeight/2-$(this).height()/2;
					break;
				case("middle")://居中
					var l=0;//居左
					var t=0;//居上
					l=windowWidth/2-$(this).width()/2;
					t=windowHeight/2-$(this).height()/2;
					top=t;
					loc={left:l+"px",top:t+"px"};
					break;
				default://默认为右下角
					location="rightbottom";
					loc={right:"0px",bottom:"0px"};
					break;
			}
		}else{
			loc=location;
			alert(loc.bottom);
			var str=loc.top;
			//09-11-5修改：加上top为空值时的判断
			if (typeof(str)!= 'undefined'){
				str=str.replace("px","");
				top=str;
			}
		}
		/***fied ie6 css hack*/
		if(isIE6){
			if (top>=0)
			{
				wrap=$("<div style=\"top:expression(documentElement.scrollTop+"+top+");\"></div>");
			}else{
				wrap=$("<div style=\"top:expression(documentElement.scrollTop+documentElement.clientHeight-this.offsetHeight);\"></div>");
			}
		}
		$("body").append(wrap);
		
		wrap.css(loc).css({position:"fixed",
			z_index:"999"});
		if (isIE6)
		{
			
			wrap.css("position","absolute");
			//没有加这个的话，ie6使用表达式时就会发现跳动现象
			//至于为什么要加这个，还有为什么要加nothing.txt这个，偶也不知道，希望知道的同学可以告诉我
			$("body").css("background-attachment","fixed").css("background-image","url(n1othing.txt)");
		}
		//将要固定的层添加到固定层里
		$(this).appendTo(wrap);
	});
};