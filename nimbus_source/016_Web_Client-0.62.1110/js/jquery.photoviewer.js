(function ($) {

	var DIV_OVERLAY = 
	'<div id="slimBoxContainer" style="z-index: 999;">' +
	'	<div class="closed"><a id="pv_close" href="javascript:void(0);">X Close</a></div>' +
	'	<div class="btnContainer"><a id="pv_left" href="javascript:void(0);" class="clickLeft"></a><a id="pv_right" href="javascript:void(0);" class="clcikRight"></a></div>' +
	'	<div id="pv_container" style="overflow: hidden;" class="showImg"></div>' +
	'	<div id="pv_pages" class="pages"></div>' +
	'	<div class="functions"><a href="javascript:void(0);">Slideshow</a> <a href="javascript:void(0);">Full Screen</a>' +
	'	</div>' +
	'</div>';

	var PV_LI = '<span style="width: #WIDTH#, height: #HEIGHT#, float:left">', PV_LI_END = '</span>';
	var PV_IMG = '<img src="#IMAGE#" />';

	var DEFAULT_OPTIONS = {
		index: 0,
		cache: false
	};
	
	var USER_OPTIONS = {};

	var viewer = [];

	var overlay = {
		container: null,

		lists: [],

		presenter: [],

		content: null,

		pages: null,

		width: 0,

		height: 0,

		__appendPresenter: function(width, height) {
			log('size: ' + width + 'x' + height);
			for (var i = 0; i < 3; i++)
			{
				this.presenter.push($('<span style="position: relative;"><img src="http://static.jquery.com/files/rocker/images/logo_jquery_215x53.gif" /></span>'));
				this.presenter[i].css({
					"left": (i - 1) * width,
					"top": 0,
					"width": width,
					"height": height
				});
				this.content.append(this.presenter[i]);
			}
		},

		create: function() {
			if (this.container == null)
			{
				$('body').append(DIV_OVERLAY);
				this.container = $('#slimBoxContainer');
				this.container.find('#pv_close').click(function() {
					overlay.hide.call(overlay);
				});
				this.container.find('#pv_left').click(this.onLeft);
				this.container.find('#pv_right').click(this.onRight);
				this.content = this.container.find('#pv_container');
			}
		},

		clear: function() {
			this.lists.length = 0;
		},

		add: function(src) {
			// this.lists.push(PV_LI + PV_IMG.replace('#IMAGE#', src) + PV_LI_END);
		},

		show: function() {
			this.content.html(this.lists.join(''));
			var width = $('body').width();
			var height = $('body').height();
			this.container.width(width);
			this.container.height(height);
			this.container.css({
				"left": 0,
				"top": 0,
				"position": "absolute"
			});

			this.__appendPresenter(width, height);
			this.container.show();
		},

		hide: function() {
			this.container.hide();
		},

		onLeft: function() {
			
		},

		onRight: function() {

		}
	};
	
	function PhotoViewer(target, options) {
		this.options = options;
		this.target = target;
		this.id = target.attr('id');
		this.body = DIV_OVERLAY;

		var images = [];
		this.show = function () {
			if (!this.options.cache)
				loadChildImages();

			overlay.clear();
			for (var i = 0; i < images.length; i++)
				overlay.add(images[i]);

			overlay.show();
		};

		function loadChildImages()
		{
			images.length = 0;
			target.children('li').each(function(i, e){
				var je = $(e);
				var je_img = je.find('img');
				if (je_img.length > 0)
				{
					var img = je_img.closest('img');
					//log(img.attr('src'));
					images.push(PV_LI + img.attr('src'));
				}
			});
		}
		
		function init()
		{
			overlay.create();
		}

		init();
	};

	var __html = [];

	function log(s)
	{
		var el;
		if ($('#debug').length == 0)
		{
			el = $(document.createElement('div')).appendTo('body');
			el.attr('id', "debug");
			el.css({
				"left": 0,
				"top": 0,
				"position": "absolute",
				"backgroundColor": "#cccccc",
				"zIndex": 1000
			});
		}
		else
			el = $("#debug");
			
		__html.push(s);
		el.html(__html.join('<br />'));
		el.css('display', "block");
	}
	
	function createPhotoViewer(i, e) {
		var target = $(e);
		var o = null;
		if (target.attr('id') == '')
		{
			log('Then element should be set an id');
			return;
		}
		for (n = 0; n < viewer.length; n++)
		{
			o = viewer[n];
			if (o.id == target.attr('id'))
			{
				o.show();
				return target;
			}
		}

		o = new PhotoViewer(target, USER_OPTIONS);
		viewer.push(o);
		o.show();
		return o.target;
	};

	$.fn.photoViewer = function ( options ) {
		$.extend(USER_OPTIONS, DEFAULT_OPTIONS, options || {});
		return $(this).each( createPhotoViewer );
	};
	
})(jQuery);