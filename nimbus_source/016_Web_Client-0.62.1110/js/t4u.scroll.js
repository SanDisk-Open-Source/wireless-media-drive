(function($){

	var bindTarget;
	var _target;
	var _callback;
	var _bound = false;
	var _percent = 75;
	var boundTarget = [];

	function onScrollEvent()
	{
		var diffHeight = _target.children(":visible").innerHeight() - $(this).innerHeight();
		var scrollTop = $(this).scrollTop();
		if ((scrollTop / diffHeight * 100) > _percent)
			triggerScrollCallback(scrollTop, diffHeight);
	}

	function triggerScrollCallback(scrollTop, totalHeight)
	{
		if (typeof _callback === 'function')
		{
			_callback.call(_target, scrollTop, totalHeight);
		}
	}

	t4u.namespace('t4u.scroll', {

		bind: function(target, callback, percent) {
			_target = target;
			_callback = callback;
			_percent = $.isNumeric(percent) ? percent : 75;

			if (!_target || _target.length <= 0)
			{
				t4u._debug('t4u.scroll: invalid target');
				return;
			}

			this.unbind();
			bindTarget = target;
			$(bindTarget).bind('scroll', onScrollEvent);
		},

		unbind: function()
		{
				$(bindTarget).unbind('scroll');
		}

	});

})(jQuery);