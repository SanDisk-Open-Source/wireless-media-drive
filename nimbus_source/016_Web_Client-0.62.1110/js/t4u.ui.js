(function($){

	t4u.namespace('t4u.ui', {

		arrowButton: '<span class="thumbnailShowRighClickArrow" style="display:none" >&nbsp;</span>',

		createCheckbox: function(data){
			var checkbox = '<input type="checkbox" class="cbSelection" value="{';
			var value = [];
			for (var o in data)
			{
				var quot = '&quot;';
				if (typeof data[o] !== 'string')
					quot = '';
					
				if (value.length > 0)
					value.push(',');

				value.push('&quot;' + o + '&quot;: ' + quot + data[o] + quot);
			}

			return checkbox + value.join('') + '}" />';
		},

		getFileType: function(extname) {
			var target = SUPPORTED_FILE_FORMAT_COMPILED;
			var search = '|' + extname.toLowerCase() + '|';
			var p = target.indexOf(search);
			if (p != -1)
			{
				p += search.length;
				var start = parseInt(target.charAt(p) + target.charAt(p + 1), 10);
				var length = parseInt(target.charAt(p + 2) + target.charAt(p + 3), 10);
				return target.substr(
						start,
						length
					);
			}

			return null;
		}

	});
	
})(jQuery);