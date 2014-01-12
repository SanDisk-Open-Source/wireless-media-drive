/**
 * 
 * This plugin allows you to rotate an DOM element (e.g. div) by the amount of degrees given.
 *
 * @param degress int 		Degrees (0-360)
 * @param options Object	Options (not yet implemented in version 0.1)
 *                  
 */
$.fn.jqrotate = function(degrees, options)
{
	var options = $.extend({ 
				animate : false // not yet implemented
				}, options);
			    
			    return this.each(function()
			    {
			        var $this = $(this);
			  
			      var oObj = $this[0];
			      var deg2radians = Math.PI * 2 / 360;
			      
			      var rad = degrees * deg2radians;
			      var costheta = Math.cos(rad);
			      var sintheta = Math.sin(rad);
			      
			      a = parseFloat(costheta).toFixed(8);
			      b = parseFloat(-sintheta).toFixed(8);
			      c = parseFloat(sintheta).toFixed(8);
			      d = parseFloat(costheta).toFixed(8);
			      
			      $this.css( {	'-ms-filter' : 'progid:DXImageTransform.Microsoft.Matrix(M11=' + a + ', M12=' + b + ', M21=' + c + ', M22=' + d + ',sizingMethod=\'auto expand\')',
			    	  			'filter' : 'progid:DXImageTransform.Microsoft.Matrix(M11=' + a + ', M12=' + b + ', M21=' + c + ', M22=' + d + ',sizingMethod=\'auto expand\')',
			    	  			'-moz-transform' :  "matrix(" + a + ", " + c + ", " + b + ", " + d + ", 0, 0)",
			    	  			'-webkit-transform' :  "matrix(" + a + ", " + c + ", " + b + ", " + d + ", 0, 0)",
			    	  			'-o-transform' :  "matrix(" + a + ", " + c + ", " + b + ", " + d + ", 0, 0)"
			      });
			        
			        
			    });  
			};