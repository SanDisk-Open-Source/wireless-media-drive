(function(){
	var aplist = {};//Ryan
	// Author: Ryan
	// Rules: 1.Only one "@"
	//        2.Only one "." and cannot appear continuely
	//        3.No blank
	function ValidEmail(emailtoCheck){
	
	var regExp = /^[^@^\s]+@[^\.@^\s]+(\.[^\.@^\s]+)+$/;
	var setting_ftp, setting_bonjour, setting_dnla = false; // set use services
	if ( regExp.test(emailtoCheck) )
		return true;
	else
		return false;
	}
	
	/***
		Author: Ryan
		Desc: Generate GUID
	*/
	function S4() { 
	   return (((1+Math.random())*0x10000)|0).toString(16).substring(1); 
	} 
	
	/***
		Author: Ryan
		Desc: Generate GUID
	*/
	function NewGuid() { 
	   return (S4()+S4()+"-"+S4()+"-"+S4()+"-"+S4()+"-"+S4()+S4()+S4()); 
	} 
	
	/***
		Author: Ryan
		Desc: Generate URL
	*/
	function TokenUrl(a) {
		token = a;
		url= xmlUrl+"?token="+token;
		return url;
	}
		
	//Radio class
	//Ian
	
	
	var settings = {
		isInitialized : false,
		/***
			Desc: init UI
		*/
		init: function()
		{
			if(this.isInitialized)
				return;
			this.isInitialized = true;
            this.BatteryLevelGetting();//Ian
			this.DeviceNameGetting();//Ian
			t4u.SS.ChangeLang();//Ian 多國語言的套用
			PasswordHindSetting.init();			
			ConnectToInternetSetting.init();			
			t4u.SS.PopupResetFactory();		
			t4u.SS.FormatNimbus.init();//Tony	
			t4u.SS.FirmwareUpgrade.init();//Tony	
			AboutSetting.init();			
			LanguageSetting.init();
			this.updateUI(this.isLogin());			
			this.bind();
		},	
		isLogin: function()
		{
			return token != '';
		},
		bind: function()
		{								
			//Steven: 一開始為什麼要隱藏？
			//設定確認密碼是否顯示,change wifi name
			//Ian
			$('#setting-wifi-popup-password').hide();
			$('#setting-wifi-popup-confirm').hide();
				
			//更新電池資訊,五秒更新一次
			setInterval(function() {
				//t4u.bgProcess=true;
				
    			t4u.SS.BatteryLevelGetting();
				//t4u.bgProcess=false;
			}, batteryRefreshDuration);
			
			t4u.SS.PopupWifiName();
			
			 $(document).keypress(function(e){
				if(e.which == 13)
				{
					if(e.target.id == 'setting-admin-password')
					{
						t4u.SS.AdminLoginSetting();
					}
					else if(e.target.id == 'setting-device-name')
					{
						t4u.SS.DeviceNameSetting();
					}
				}
			});
			
			
		},
		updateUI: function(bLogin)
		{
			$('.lblBtn').css('cursor', bLogin ? 'pointer' : 'default');
			if(bLogin)
			{
				mysetting.init();//Ryan
				
				$('#divLogin').hide();
				$('#divLogout').show();				
				
				//點選反應msc on or off
				$('#setting-msc-on').click(function(){
						$("#setting-msc-on").addClass(cssRadioButtonSelected);
						$("#setting-msc-off").removeClass(cssRadioButtonSelected);
						t4u.blockUIMsg();
						t4u.nimbusApi.setting.setUseServices(function(data){
							t4u.unblockUI();
							if(t4u.nimbusApi.getErrorCode(data) == 0){	
								t4u.showMessage(multiLang.search('MscOpen'));
							}
							else
							{
								t4u.showMessage(multiLang.search('MscFail'));
							}
							}, function(data){t4u.unblockUI();}, {MSCMode: true,DNLA: false, Bonjour: false, FTP: true, internetAP: ConnectToInternetSetting.setting_internetAP }, xmlUrl+"?token="+token);
						GetSetting.setting_mscmode = true;
					});			
					
				$('#setting-msc-off').click(function(){
						$("#setting-msc-off").addClass(cssRadioButtonSelected);
						$("#setting-msc-on").removeClass(cssRadioButtonSelected);
						t4u.blockUIMsg();
						t4u.nimbusApi.setting.setUseServices(function(data){
							t4u.unblockUI();
							if(t4u.nimbusApi.getErrorCode(data) == 0){	
								t4u.showMessage(multiLang.search('MscClose'));
							}
							else
							{
								t4u.showMessage(multiLang.search('MscFail'));
							}
						}, function(data){t4u.unblockUI();}, {MSCMode: false,DNLA: false, Bonjour: false, FTP: true, internetAP: ConnectToInternetSetting.setting_internetAP }, xmlUrl+"?token="+token);
						GetSetting.setting_mscmode = false;
					});			
				
				function checked_password_confirm_value(){
					
					if($('#setting-wifi-popup-password').is(':hidden'))
					{
						$('#setting-wifi-popup-password').val($('#setting-wifi-popup-password-hide').val());
					}
					else//if($('#setting-wifi-popup-password-hide').is(":hidden"))
					{
						$('setting-wifi-popup-password-hide').val($('#setting-wifi-popup-password').val());
					}
				
					if($('#setting-wifi-popup-confirm').is(":hidden"))
					{
						$('#setting-wifi-popup-confirm').val($('#setting-wifi-popup-confirm-hide').val());
					}
					else//if($('#setting-wifi-popup-confirm-hide').is(":hidden"))
					{
						$('#setting-wifi-popup-confirm-hide').val($('#setting-wifi-popup-confirm').val());
					}			
				}

				function init_setting_reset_factory()
				{					
					var popupResetWiFiName = $('#setting-resetFactory-popup-ResetWiFiName'),
						popupResetWiFiOpenPassword=$('#setting-resetFactory-popup-ResetWiFi-OpenPassword'),
						popupResetAdmin = $('#setting-resetFactory-popup-ResetAdmin'),
						popupEliminate= $('#setting-resetFactory-popup-Eliminate');
						
					popupResetWiFiName.find('label').removeClass(cssRadioButtonSelected);
						popupResetWiFiOpenPassword.find('label').removeClass(cssRadioButtonSelected);
						popupResetAdmin.find('label').removeClass(cssRadioButtonSelected);
						popupEliminate.find('label').removeClass(cssRadioButtonSelected);
				}
				
				$('#setting-reset-factory').click(function(){
					t4u.blockUI($('#setting-resetFactory-popup'));
					init_setting_reset_factory();
				});

				$(".authority").removeClass("disable");	
				
				//popup setting wifi
				$('#setting-wifi-name').click(function(){
					t4u.blockUI($('#setting-wifi-popup'));
				});
				
				//設定click事件
				$('#setting-format-nimbus').click(function(){
					t4u.bgProcess=false;
					FormatNimbusSettings.initValue();
					t4u.nimbusApi.setting.getNimbusInfo(function(data){
						if(t4u.nimbusApi.getErrorCode(data) == 0)
						{
							if(typeof data.nimbus.storages.storageitem[1] != 'undefined') {
								$('#formatSDCard').removeClass('hidden');
								$('[name="formatOptions[]"][value="sdcard"]').attr('checked', true);
							}
							else {
								$('#formatSDCard').addClass('hidden');
								$('[name="formatOptions[]"][value="sdcard"]').attr('checked', false);
							}
						}
					},function(data){
					})
					t4u.bgProcess=true;
					t4u.blockUI($('#dv-Confirm-format')); 
				});	
				
				//設定click事件
				$('.setting-check-firmware-upgrade').click(function(){
					if(settings.isLogin())
					{
						//t4u.blockUI();
						default_api_timeout = timeout.long; // more execute time 
						t4u.blockUIMsg();
						t4u.nimbusApi.setting.getFirmwareInfo(function(data){
							if(t4u.nimbusApi.getErrorCode(data) == 0 && typeof data.nimbus.newVersion === 'string')
							{						
								t4u.blockUI($('#dv-confirm-firmware-upgrade'));//顯示確認更新視窗	
							}							
							else
							{
								t4u.unblockUI();
								var objMsg = {};
								objMsg.title = multiLang.search('firmware-upgrade-title');
								objMsg.message = multiLang.search('FirmwareIsLastVersion').replace('{0}', data.nimbus.currentVersion);
								t4u.showMessage(objMsg);
							}
							default_api_timeout = timeout.short; // set to default value
						},
						function(data){
						},
						url);				
					}
					else
					{
						t4u.showMessage(multiLang.search('Please login using your password'));
					}				
				});
			}
			else
			{
				//hide login div
				$('#divLogin').show();
				//show logout div
				$('#divLogout').hide();
				
				$('#setting-device-name').val('');
				$('.authority').addClass('disable');
				
				//清掉token
				$('#setting-admin-password').attr('disabled', false);
				token = '';
				url= '';
				$('#setting-admin-password').val('');
				
				//checkbox改成灰色的
				$("#setting-msc-off").removeClass(cssRadioButtonSelected);
				$("#setting-msc-on").removeClass(cssRadioButtonSelected);
				
				//unbind所有的按鈕
				//$('#setting-who-connected').unbind('click');
				$('#btWhoConnectedBack').unbind('click');
				$('#setting-msc-on').unbind('click');
				$('#setting-msc-off').unbind('click');
				$('#setting-confirm-hide').unbind('toggle');
				$('#setting-reset-factory').unbind('click');
				$('#setting-change-admin-password-popup').unbind('click');
				$('#setting-wifi-name').unbind('click');
				//$('#setting-connect-to-internet-popup').unbind('click');
				$('#setting-subscribe-popup').unbind('click');
				$('#setting-format-nimbus').unbind('click');
				$('.setting-check-firmware-upgrade').unbind('click');
			}
		},		
		//popup reset factory
		PopupResetFactory : function()
		{
			function reset_value()
			{
					resetWiFiName = false;
				openPassword = false;
				resetAdmin = false;
				eliminate = false;			
			}
					
			var resetWiFiName = false,
				openPassword = false,
				resetAdmin = false,
				eliminate = false,				
				popupResetWiFiName = $('#setting-resetFactory-popup-ResetWiFiName'),
				popupResetWiFiOpenPassword=$('#setting-resetFactory-popup-ResetWiFi-OpenPassword'),
				popupResetAdmin = $('#setting-resetFactory-popup-ResetAdmin'),
				popupEliminate= $('#setting-resetFactory-popup-Eliminate');
				
				popupResetWiFiName.click(function(){
					if(!resetWiFiName)
						popupResetWiFiName.find('label').addClass(cssRadioButtonSelected);
					else
						popupResetWiFiName.find('label').removeClass(cssRadioButtonSelected);
					resetWiFiName = !resetWiFiName;
				});
				//setting-resetFactory-popup-ResetWiFi-OpenPassword
				popupResetWiFiOpenPassword.click(function(){
					if(!openPassword)
						popupResetWiFiOpenPassword.find('label').addClass(cssRadioButtonSelected);
					else
						popupResetWiFiOpenPassword.find('label').removeClass(cssRadioButtonSelected);					
					openPassword = !openPassword;
				});
				//setting-resetFactory-popup-ResetAdmin
				popupResetAdmin.click(function(){
					if(!resetAdmin)
						popupResetAdmin.find('label').addClass(cssRadioButtonSelected);
					else
						popupResetAdmin.find('label').removeClass(cssRadioButtonSelected);
					resetAdmin = !resetAdmin;
				});
				//setting-resetFactory-popup-Eliminate
				popupEliminate.click(function(){
					if(!eliminate)
						popupEliminate.find('label').addClass(cssRadioButtonSelected);
					else
						popupEliminate.find('label').removeClass(cssRadioButtonSelected);
					eliminate = !eliminate;
			});

			$('#setting-resetFactory-popup-ok').click(function(){
					if(resetWiFiName == false && openPassword == false && resetAdmin == false && eliminate == false)
						{
							var word=multiLang.search('Please select at least one item');
							t4u.showMessage(word);
						}
						else
						{
						/*
							 t4u.showMessage(word);
							 */
							var word=multiLang.search('Reset selected item(s) to factory default?');
							t4u.confirm({
								message:word,
								txtYes:multiLang.search('Yes'),
								txtNo:multiLang.search('Cancel'),
								eventbtnYes:function(){
									default_api_timeout = timeout.long; // more execute time 
									t4u.blockUIMsg();
									t4u.nimbusApi.setting.resetNimbus(function(data){
										t4u.unblockUI();
										if(t4u.nimbusApi.getErrorCode(data) == 0)
										{
											GetSetting.init(); //init setting layout
											t4u.showMessage(multiLang.search('ResetSuccess'));
										}
										else
										{
											t4u.showMessage(multiLang.search('ResetFail'));
										}
										default_api_timeout = timeout.short; // set to default value
										reset_value();
									},
									function(data){
										reset_value();
										t4u.unblockUI();
									}
									,{ssidName:resetWiFiName,ssidPassword:openPassword,adminPassword:resetAdmin,deleteAllAPlist:eliminate}, url);
								}
								
							});
						}
				});
			
			//關閉reset factory
			$('#setting-resetFactory-popup-cancel').click(function(){
				t4u.unblockUI($('#setting-resetFactory-popup'));			
				reset_value();
			});
			
		},
		//popup wifi name
		PopupWifiName : function ()
		{
			var showFirstPasswordCheck = false,
				showSecondPasswordCheck = false,
				current = this;
				
			bind();
				
			function checked_password_confirm_value(){
				
				if($('#setting-wifi-popup-password').is(":hidden"))
				{
					$('#setting-wifi-popup-password').val($('#setting-wifi-popup-password-hide').val());
				}
				else//if($('#setting-wifi-popup-password-hide').is(":hidden"))
				{
					$('#setting-wifi-popup-password-hide').val($('#setting-wifi-popup-password').val());
				}

				if($('#setting-wifi-popup-confirm').is(":hidden"))
				{
					$('#setting-wifi-popup-confirm').val($('#setting-wifi-popup-confirm-hide').val());
				}
				else//if($('#setting-wifi-popup-confirm-hide').is(":hidden"))
				{
					$('#setting-wifi-popup-confirm-hide').val($('#setting-wifi-popup-confirm').val());
				}			
			}
				
			
			function init_value()
			{
				//$('#ssid_name').val('');
				$('#setting-security-select').text('NONE');
				$('.passwordBox').addClass('hidden');
				$('#setting-wifi-popup-password').val('');
				$('#setting-wifi-popup-confirm').val('');
				$('#setting-password-hide').css({'background':' url(data/images/icons/icon_grayCheckBox.png) no-repeat 5px 5px '});
				$('#setting-confirm-hide').css({'background':' url(data/images/icons/icon_grayCheckBox.png) no-repeat 5px 5px '});
			}
			
			function bind()
			{
				$('#setting-password-hide').click(function(){
					var newpass1 = $('#setting-wifi-popup-password-hide'),
						newpass2 = $('#setting-wifi-popup-password');
					if(!showFirstPasswordCheck)
					{
						$(this).css({'background':' url(data/images/icons/icon_grayCheckBoxChecked.png) no-repeat 5px 5px '});
				newpass1.hide();
				newpass2.val(newpass1.val());
				newpass2.show();
					}
					else
					{
						$(this).css({'background':' url(data/images/icons/icon_grayCheckBox.png) no-repeat 5px 5px '});
				newpass2.hide();
				newpass1.val(newpass2.val());
				newpass1.show();
				}
					showFirstPasswordCheck = !showFirstPasswordCheck;
				});
			
				$('#setting-confirm-hide').click(function(){
					var newpass1 = $('#setting-wifi-popup-confirm-hide'),
						newpass2 = $('#setting-wifi-popup-confirm');
					if(!showSecondPasswordCheck)
					{
				$(this).css({'background':' url(data/images/icons/icon_grayCheckBoxChecked.png) no-repeat 5px 5px '});
				newpass1.hide();
				newpass2.val(newpass1.val());
				newpass2.show();
					}
					else
					{
				$(this).css({'background':' url(data/images/icons/icon_grayCheckBox.png) no-repeat 5px 5px '});
				newpass2.hide();
				newpass1.val(newpass2.val());
				newpass1.show();
				}	
					showSecondPasswordCheck = !showSecondPasswordCheck;
				});
			
			//選擇Security 狀態
			$('#setting-security-select-open').click(function(){
				$('#setting-security-select').text($('#setting-security-select-open').html());
				$('.passwordBox').addClass('hidden');
			});
			$('#setting-security-select-wepkey').click(function(){
				$('#setting-security-select').text($('#setting-security-select-wepkey').html());
				$('.passwordBox').removeClass('hidden');
				var word=multiLang.search('Password must be 5 or 13 characters');
				$('#setting-wifi-popup-pwlength, #setting-wifi-popup-conlength').text(word);
			});
			$('#setting-security-select-wap').click(function(){
				$('#setting-security-select').text($('#setting-security-select-wap').html());
				$('.passwordBox').removeClass('hidden');
				var word=multiLang.search('Password must have at least 8-63 characters');
				$('#setting-wifi-popup-pwlength, #setting-wifi-popup-conlength').text(word);
			});
			$('#setting-security-select-wap2').click(function(){
				$('#setting-security-select').text($('#setting-security-select-wap2').html());
				$('.passwordBox').removeClass('hidden');
				var word=multiLang.search('Password must have at least 8-63 characters');
				$('#setting-wifi-popup-pwlength, #setting-wifi-popup-conlength').text(word);
			});			
			
			//wifi name password 判斷密碼長度
			$('#setting-wifi-popup-ok').click(function(){
				checked_password_confirm_value();
			
				var check_all_fields = true;
					
					//Don't check the length of SSID length
					/*
				if(!$.trim($('#ssid_name').val())) {
					t4u.showMessage(multiLang.search('Incomplete response'));
					return false;
				}
					*/
				
					/*
					if($('#setting-security-select').text() == 'WEP' || $('#setting-security-select').text() == 'WPA') {
					if(!$.trim($('#setting-wifi-popup-password-hide').val()) || !$.trim($('#setting-wifi-popup-confirm-hide').val())) {
							t4u.showMessage(multiLang.search('WifiPasswordError'));
						return false;
					}
				}
					*/
					
				if($('#setting-wifi-popup-password').is(":hidden") && $('#setting-wifi-popup-confirm').is(":hidden"))
				{
					var password = $('#setting-wifi-popup-password-hide').val();
					var confirm  = $('#setting-wifi-popup-confirm-hide').val();
				}
				else if($('#setting-wifi-popup-password-hide').is(":hidden") && $('#setting-wifi-popup-confirm').is(":hidden"))
				{
					var password = $('#setting-wifi-popup-password').val();
					var confirm  = $('#setting-wifi-popup-confirm-hide').val();
				}
				else if($('#setting-wifi-popup-password').is(":hidden") && $('#setting-wifi-popup-confirm-hide').is(":hidden"))
				{
					var password = $('#setting-wifi-popup-password-hide').val();
					var confirm  = $('#setting-wifi-popup-confirm').val();
				}
				else
				{
					var password = $('#setting-wifi-popup-password').val();
					var confirm  = $('#setting-wifi-popup-confirm').val();
				}
					if($('#setting-security-select').text() == 'WEP' && (password.length != 5 || confirm.length != 5) && (password.length != 13 || confirm.length != 13)) // if Wep
				{
					word=multiLang.search('Password must be 5 or 13 characters');
					t4u.showMessage(word);
				}
				else if($('#setting-security-select').text() == 'WPA' && ((password.length<8 || password.length>63 ) || (confirm.length<8 || confirm.length>63 ))) // if WPA
				{
					word=multiLang.search('Password must have at least 8-63 characters');
					t4u.showMessage(word);
				}
				else
				{
						if($('#setting-security-select').text() != 'NONE' && password != confirm)
					{
						var word=multiLang.search('Password and Confirm password have to be the same');
						t4u.showMessage(word);
					}
					else
					{
							var ssid_security = $('#setting-security-select').text() == 'WEP'?'wep':$.trim($('#setting-security-select').text().toLowerCase());
							t4u.blockUIMsg();
						t4u.nimbusApi.setting.setSSID(function(data){
							t4u.unblockUI();
								if(t4u.nimbusApi.getErrorCode(data) == 0)
									t4u.showMessage(multiLang.search('ChangeSSIDSuccess'));
						},
						function(data){
						},
						{name: $.trim($('#ssid_name').val()),password:password, security: ssid_security}, TokenUrl(token));
					}
					
				}
			});
			
			//關閉popup setting wifi
			$('#setting-wifi-popup-cancel').click(function(){
					t4u.unblockUI();
					init_value();
			});
			}
		},
		/***
			Author:Ian
			Desc:語言初始化
		*/
		ChangeLang : function()
		{
			//change MSC Mode lang
			var word_off = multiLang.search('Off');
			$('#setting-msc-off').text(word_off);
			var word_on = multiLang.search('On');
			$('#setting-msc-on').text(word_on);
			
			//change Enable Server Functionality Mode lang
			var word_mode_1 = multiLang.search('DNLA and PnP Media Server');
			var word_mode_2 = multiLang.search('Bonjour Media Server');
			var word_mode_3 = multiLang.search('FTP Server');
			$('#setting-MscMode-DNLA').text(word_mode_1);
			$('#setting-MscMode-Bonjour').text(word_mode_2);
			$('#setting-MscMode-FTP').text(word_mode_3);
			
			//chage wifi name popup lang
			var word_msg_w1 = multiLang.search('Configure the Wireless Media Drive Network');
			var word_msg_w2 = multiLang.search('Wireless Network Name (WiFi SSID)');
			var word_show_characters = multiLang.search('show characters');
			$('#setting-wifi-meassage').html(word_msg_w1 + '<br / >' + word_msg_w2);
			$('#setting-password-hide').text(word_show_characters);
			$('#setting-confirm-hide').text(word_show_characters);
			
			//Reset to Factory Settings language
			var word_msg_f1 = multiLang.search('Choose the items you would like to reset back to factory settings.');
			var word_msg_f2 = multiLang.search('This will not erase your files.');
			$('#setting-resetFactory-message').html(word_msg_f1 + '<br / >' + word_msg_f2);
			
			//format multi-language(tony)
			$('#setting-format-nimbus').text(multiLang.search('Format Media Drive'));
			$('#formatConfirmMessage').text(multiLang.search('FormatConfirmMessage'));
			$('.formatConfirmTitle').text(multiLang.search('FormatConfirmTitle'));
			$('#formatingMessage').text(multiLang.search('Formatting in progress'));
			
			//Firmware Upgrade(tony)
			$('#confirm-firmware-upgrade-title').text(multiLang.search('firmware-upgrade-title'));
			$('#confirm-firmware-upgrade-message').text(multiLang.search('confirm-firmware-upgrade-message'));
			$('#firmware-upgrade-title').text(multiLang.search('firmware-upgrade-title'));
			//$('#firmware-upgrade-message').text(multiLang.search('firmware-upgrade-message'));						
			
			//who connect(tony)
			/*
			$('#setting-who-connected').text(multiLang.search('setting-who-connected'));
			$('#who-connected-title').text(multiLang.search('who-connected-title'));
			*/
			
			
			//base multi-language(tony)
			for(var i =0; i < $('.btnCancel').length;i++)
			{
				$('.btnCancel')[i].value =multiLang.search('Cancel');
			}
			for(var i=0;i < $('.btnOK').length; i++)
			{
				$('.btnOK')[i].value =multiLang.search('OK');
			} 
			 
			
		},
		//設定DeviceName
		//Ian
		DeviceNameSetting: function()
		{
			var deviceName=$('#setting-device-name').val();
			if(jQuery.browser.safari)
				deviceName = "Safari";
			else if(jQuery.browser.msie)
				deviceName = "IE";
			else if(jQuery.browser.mozilla)
				deviceName = "Firefox";
			else if(jQuery.browser.chrome)
				deviceName = "Chrome";
			
			if(deviceName != "" )
			{					
				t4u.nimbusApi.setting.changeDeviceName(function(data){					
					setTimeout(function(){
						$('#btnFolderView').trigger('click');
					}, 100);
				},
				function(data){
					setTimeout(function(){
							$('#btnFolderView').trigger('click');
						}, 100);
				},
				{name:deviceName},xmlUrl);
			}
		},
		//取得DeviceName
		//Ian
		DeviceNameGetting: function()
		{
			t4u.nimbusApi.setting.getDeviceName(function(data){
				if(t4u.nimbusApi.getErrorCode(data) == 0)
				{
					if(typeof(data.nimbus.user.name) == "object")
					{
						$('#setting-device-name').val("");
						
					}						
					else
					{
						$('#setting-device-name').val(data.nimbus.user.name);
					}
				}
				},function(data){
				});
		},
		//取得BattyLevel
		//Ian
		BatteryLevelGetting: function()
		{
			var batteryLevel='#divBatteryLevel';
			var level = "";
			t4u.bgProcess=true;
			t4u.nimbusApi.setting.getBatteryLevel(function(data){
					if(t4u.nimbusApi.getErrorCode(data) == 0)
					{						
						level = data.nimbus.level + '%'
						if(data.nimbus.status == 'charge')
							$(batteryLevel).text(multiLang.search('Charging'));
						else
							$(batteryLevel).text(level);
						
						$('#batteryProgressRateBar').css('width',level);
						
					}
					t4u.bgProcess=false;
			},
			function(data){
			});
		},
		//儲存msc狀態
		
		//登入AdminLogin
		//Ian
		AdminLoginSetting: function()
		{
			var setPassword = $('#setting-admin-password').val();
			t4u.blockUIMsg();
			t4u.nimbusApi.setting.adminLogin(function(data){
				if(t4u.nimbusApi.getErrorCode(data) == 0)//login sucessfully
				{
					token = data.nimbus.token;
					url= xmlUrl + "?token=" + token;
					//其他反白的部分開啟
					t4u.SS.updateUI(true);
					$('#setting-admin-password').attr('disabled', true); // prevent from user relogining					
					t4u.showMessage(multiLang.search('LoginSuccess'));
				}
				else
				{
					var word =multiLang.search('Incorrect password, please try again');
					t4u.showMessage(word);
					token="";
				}				
				t4u.unblockUI();
				$('html, body').css('overflow', 'hidden'); 
			},
			function(data){
			},
			{password:setPassword});
		},
		
		//設定目前連線者的資訊
		WhoConnectedInfoSetting: function()
		{
			t4u.nimbusApi.setting.getConnectedDevices(function(data){
				if(t4u.nimbusApi.getErrorCode(data) == 0)
				{
					var td = "", tr = "", table = "";
					var imgOn="<img src='" + webImageUrl + "icons/blueRadioBtn02.png'" + ' />';
					var imgOff="<img src='" + webImageUrl + "icons/whiteRadioBtn02.png'" + ' />';
					//清空表格
					$('#dv-connected-Info-table').empty()
					//表頭
					tr += " <tr  class='trBorderBottomLine'> <td width='80%'>"+ multiLang.search('User Names') +"</td> <td width='10%'>"+ multiLang.search('Admin') +"</td> <td width='10%'>"+ multiLang.search('Copy') +"</td> </tr>";
							
					// 如果只有一個人在線上的話，data.nimbus.user為object改為array
					//data.nimbus.user = data.nimbus.user[0];
					if(typeof data.nimbus.user.length == 'undefined') {
						var _user = data.nimbus.user;
						//delete data.numbus.user;
						data.nimbus.user = [];
						data.nimbus.user.push(_user);
					}

					for(var i=0; i<data.nimbus.user.length; i++)
					{  

						// 目前抓不到真正的名字，暫時用User 1, 2, 3
						var name = $.isEmptyObject(data.nimbus.user[i].name)?multiLang.search('Unknown'):data.nimbus.user[i].name;

						//用戶
						td += '<td>' + name + '</td>';
						
						//1＝admin  0= guest (之後再改成enum)
						var group = 0;
						if(typeof data.nimbus.user[i].group == 'object')
							group = data.nimbus.user[i].group['#text'];
						else
							group = data.nimbus.user[i].group;
						if(group == 1)
						{
							td += '<td>' + imgOn + '</td>';
						}
						else
						{
							td += '<td>' + imgOff + '</td>';
						}
			
						//是否為copy狀態
						if(data.nimbus.user[i].statusCopy)
						{
							td += '<td>' + imgOn + '</td>';
						}else
						{
							td += '<td>' + imgOff + '</td>';
						} 
						
						tr += '<tr>' + td + '</tr>';	
						td = "";			 	 
					}
					table = '<table width="100%" border="0" cellspacing="0" cellpadding="0">' + tr + '</table>';
							tr = "";
							
					$('#dv-connected-Info-table').append(table);
				}
			}, 
			function(data){
			},
			url); 
		}		
	};
	window.t4u.SS = settings;
	
	
	
	
	//Firmware Upgrade
	var FirmwareUpgradeSettings = {
		UpgradeTiemr: null,
		
		init: function()
		{
			this.bind();
		},
		bind: function()
		{
			//取消更新
			$('#btnFirmwareUpgrdeCancel').click(function(){
				t4u.unblockUI();	
				//t4u.SS.FirmwareUpgrade.stopInterval();
			});
			//確認更新
			$('#btnFirmwareUpgrdeOK').click(function(){
				if(settings.isLogin())
				{
					//t4u.blockUI($('#dv-firmware-upgrading'));
					//t4u.SS.FirmwareUpgrade.startInterval();  //開啟Timer
					t4u.unblockUI();	//關閉視窗
					//執行更新韌體的API
					t4u.showMessage(multiLang.search('FirmwareUpgrdeWaitingMsg'));
					default_api_timeout = timeout.long; // more execute time 
					
					t4u.nimbusApi.setting.upgradeFirmware(function(data){
						if(t4u.nimbusApi.getErrorCode(data) != 0)
						{
							t4u.showMessage(multiLang.search('UpgradeFirmwareFail'));
						}

					},function(data){
					},url);			
					
					t4u.SS.updateUI(false);
					
					default_api_timeout = timeout.short; // set to default value
				}
				else
				{
					t4u.showMessage(multiLang.search('Please login using your password'));
				}	
			});
		},
		
		//版本更新的Timer
		//Start Timer
		startInterval: function()
		{
			var ProgressbarValue=0;
			 
			t4u.SS.FirmwareUpgrade.UpgradeTiemr = window.setInterval(function(){	
					ProgressbarValue = ProgressbarValue+2;
					if(ProgressbarValue < 95)
					{
						$("#UpgradeProgressBar").progressbar({
							value: ProgressbarValue
						});
					}
					 
					
			}, 1000);
		},
		
		//Stop Timer
		stopInterval: function()
		{
			window.clearInterval(t4u.SS.FirmwareUpgrade.UpgradeTiemr);
		}
		
	};
	window.t4u.SS.FirmwareUpgrade = FirmwareUpgradeSettings;
	
	
	//Format
	var FormatNimbusSettings = {
		FormatTiemr: null,
		
		init: function()
		{
			this.bind();
		},
		bind: function()
		{
			var dataJson={internal:true,sd:true};
			//取消format
			$('#btnFormatCancel').click(function(){
				t4u.unblockUI();	 
			});
			//確認format
			$('#btnFormatOK').click(function(){
				dataJson={internal:$('[name="formatOptions[]"][value="storage"]').is(':checked'),sd:$('[name="formatOptions[]"][value="sdcard"]').is(':checked')};
				//t4u.unblockUI();
				t4u.blockUI($('#dv-formating'));
				t4u.SS.FormatNimbus.startInterval();
				
				t4u.nimbusApi.setting.formatNimbus(function(data){
					if(t4u.nimbusApi.getErrorCode(data) == 0)
					{
						t4u.showMessage(multiLang.search('FormatCompleteMsg')); 
					}
					else
					{
						t4u.showMessage(multiLang.search('FormatFail'));	
					}
					t4u.unblockUI();	
				}, 
				function(data){
				}, 
				dataJson, url);
				 
			});
			//cancel formating
			$('#btnFormatingCancel').click(function(){
				t4u.unblockUI();	
				FormatNimbusSettings.stopInterval();
			});
			// internal & sdcard checkbox
			$('.formatCheck').click(function() {
				var index = $('.formatCheck').index($(this));
				if($('.formatCheck').eq(index).hasClass('checked')) {
					$('.formatCheck').eq(index).removeClass('checked');
					$('[name="formatOptions[]"]').eq(index).attr('checked', false);
				}
				else {
					$('.formatCheck').eq(index).addClass('checked');
					$('[name="formatOptions[]"]').eq(index).attr('checked', true);
				}
			});
		},
		
		//版本更新的Timer
		//Start Timer
		startInterval: function()
		{
			var ProgressbarValue=0;
			 
			t4u.SS.FormatNimbus.FormatTiemr = window.setInterval(function(){	
					ProgressbarValue = ProgressbarValue+2;
					if(ProgressbarValue < 95)
					{
						$("#FormatNimbusBar").progressbar({
							value: ProgressbarValue
						});
					}
					 
					
			}, 1000);
		},
		
		//Stop Timer
		stopInterval: function()
		{
			window.clearInterval(t4u.SS.FormatNimbus.FormatTiemr);
		},
		initValue: function()
		{		
			$('.formatCheck').addClass('checked');
			$('[name="formatOptions[]"]').attr('checked', true);			
		}
	};
	window.t4u.SS.FormatNimbus = FormatNimbusSettings;
		
	//檢查E-mail格式
	function ValidEmail(emailtoCheck){
	var regExp = /^[^@^\s]+@[^\.@^\s]+(\.[^\.@^\s]+)+$/;
	if ( regExp.test(emailtoCheck) )
		return true;
	else
		return false;
	}
	//GUID產生
	function S4() { 
	   return (((1+Math.random())*0x10000)|0).toString(16).substring(1); 
	} 
	function NewGuid() { 
	   return (S4()+S4()+"-"+S4()+"-"+S4()+"-"+S4()+"-"+S4()+S4()+S4()); 
	} 
	//產生登入URL(Token)
	function TokenUrl(a) {
		token = a;
		url= xmlUrl+"?token="+token;
		return url;
	}
	var  mainsetting = {	
		init: function() {
			GetSetting.init();
			ChangeAdminPasswordSetting.init();			
			LogoutSetting.init();			
			this.bind();
		},
		bind: function(){
		
		}
	};
	/*** 
		Author: Ryan
		Desc: Handle Password Hint
	*/
	var PasswordHindSetting = {
		init: function(){
			this.bind();
		},
		bind: function(){
			/*** 
			模擬登入
			*/
			// t4u.nimbusApi.setting.adminLogin(function(data){
				// token = data.nimbus.token;
				// url= xmlUrl+"?token="+token;
			// },{password:'admin',unique:NewGuid()});  
		
			var bHasHint = false;
			/***
			跳出畫面按鈕
			*/
			$('#setting-forgot-password-popup').click(function(){
				$('#setting-password-hint-popup-p').empty();
				t4u.bgProcess=false;
				t4u.nimbusApi.setting.getAdminInfo(function(data){
						if(t4u.nimbusApi.getErrorCode(data) == 0 && typeof data.nimbus.hint == 'string'){	
							bHasHint = true;
							$('#hint-answer-block').show();
							$('#setting-password-hint-popup-p2').show();
							//if(typeof data.nimbus.hint == 'string') { // detect if empty object
								var word =multiLang.search('Your password hint is');
								$('#setting-password-hint-popup-p-hint').html(word+":"+data.nimbus.hint+"<br /><br />");
							//}
						}
						else{
							bHasHint = false;
							$('#hint-answer-block').hide();
							$('#setting-password-hint-popup-p2').hide();
							var word = multiLang.search('Please refer to the quick start guide for the default password');
							$('#setting-password-hint-popup-p-hint').html(word + "<br /><br />");
						}
				},
				function(data){
				},
				{});
				t4u.bgProcess=true;
				t4u.blockUI($('#setting-password-hint-popup-window'));
			});
			/***
			按下PaswordHint的OK按鈕
			*/
			$('#setting-password-hint-popup-OK').click(function(){//按鈕ID
				if(bHasHint)
				{
					if(!$('#hint-answer').val()) {
						t4u.showMessage(multiLang.search('Incomplete response'));
						return false;
					} else {
						t4u.nimbusApi.setting.getAdminInfo(function(data){
							if(t4u.nimbusApi.getErrorCode(data) == 0 && typeof data.nimbus.password != 'undefined'){
								t4u.showMessage(multiLang.search('Admin Password') + ':' + data.nimbus.password);
							} else {
								t4u.showMessage(multiLang.search('Incorrect answer'));
							}
						},
						function(data){
						},
						{hintAnswer:$('#hint-answer').val()});
					}
				}
				$('#setting-password-hint-popup-p').empty();//清空提示
				t4u.unblockUI();
			});
			//關閉hint 的對話視窗
			$('#setting-password-hint-popup-cancel').click(function(){
				t4u.unblockUI('#setting-password-hint-popup-window');
			});
		}
	};
	/*** 
		Author: Ryan
		Desc: Handle Change Admin Password
	*/
 	var ChangeAdminPasswordSetting = {
		showFirstPassword:false,
		showSecondPassword:false,
		init: function(){
			this.bind();
		},
		bind: function(){
			
			//跳出畫面
			
			$('#setting-change-admin-password-popup').click(function(){//按鈕ID
				var word = multiLang.search('show characters');
				$('#setting-change-admin-password-checkbox1').text(word);
				$('#setting-change-admin-password-checkbox2').text(word);
				var newpass1 = $('#setting-change-admin-password-newpassword1');//NewPassword1
				var newpass2 = $('#setting-change-admin-password-newpassword2');//NewPassword2
				var cfpass1 = $('#setting-change-admin-password-confirm1');//ConfirmPassword1
				var cfpass2 = $('#setting-change-admin-password-confirm2');//ConfirmPassword2
				newpass1.show();
				cfpass1.show();
				newpass2.hide();
				cfpass2.hide();
				t4u.bgProcess=false;
				t4u.nimbusApi.setting.getAdminInfo(function(data){
				if(t4u.nimbusApi.getErrorCode(data) == 0){
					if(!$.isEmptyObject(data.nimbus.hint)) { // detect if empty object
						$('#setting-change-admin-password-popup-hint').val(data.nimbus.hint);
					}
					if(!$.isEmptyObject(data.nimbus.hintAnswer)) { // detect if empty object
						$('#setting-change-admin-password-popup-hint-answer').val(data.nimbus.hintAnswer);
					}
				}
				},function(){
				},
				{},url);
				t4u.bgProcess=true;
				t4u.blockUI($('#setting-change-admin-password-popup-window'));//跳出畫面的DIV
			});
			
			//取消並清除欄位
			
			$('#setting-change-admin-password-popup-cancel').click(function(){//Cancel ID
				t4u.unblockUI();
				ChangeAdminPasswordSetting.clearData();
			});
			
			//show密碼1
			
			$('#setting-change-admin-password-checkbox1').click(function(){ //CheckBox1 ID(span)
				var newpass1 = $('#setting-change-admin-password-newpassword1'),
					newpass2 = $('#setting-change-admin-password-newpassword2');
				if(!ChangeAdminPasswordSetting.showFirstPassword)
				{
				$(this).css({'background':' url(data/images/icons/icon_grayCheckBoxChecked.png) no-repeat 5px 5px '});//打勾圖出現
				newpass1.hide();
				newpass2.val(newpass1.val());
				newpass2.show();
				}				
				else
				{
				$(this).css({'background':' url(data/images/icons/icon_grayCheckBox.png) no-repeat 5px 5px '});//消失
				newpass2.hide();
				newpass1.val(newpass2.val());
				newpass1.show();
				}
				ChangeAdminPasswordSetting.showFirstPassword = !ChangeAdminPasswordSetting.showFirstPassword;
			});
			
			//show密碼2
			
			$('#setting-change-admin-password-checkbox2').click(function(){//CheckBox2(span)
				var cfpass1 = $('#setting-change-admin-password-confirm1'),
					cfpass2 = $('#setting-change-admin-password-confirm2');
				if(!ChangeAdminPasswordSetting.showSecondPassword)
				{
				$(this).css({'background':' url(data/images/icons/icon_grayCheckBoxChecked.png) no-repeat 5px 5px '});//打勾圖出現
				cfpass1.hide();
				cfpass2.val(cfpass1.val());
				cfpass2.show();
				}
				else
				{
				$(this).css({'background':' url(data/images/icons/icon_grayCheckBox.png) no-repeat 5px 5px '});//消失
				cfpass2.hide();
				cfpass1.val(cfpass2.val());
				cfpass1.show();
				}
				ChangeAdminPasswordSetting.showSecondPassword = !ChangeAdminPasswordSetting.showSecondPassword;								  
			});
			
			//按下OK並檢核
			
			$('#setting-change-admin-password-popup-OK').click(function(){//OK ID
				var oldpassword = $('#setting-change-admin-password-oldpassword');//OldPassword ID
				var newpassword1 = $('#setting-change-admin-password-newpassword1');//NewPassword1 ID
				var confirmpassword1 = $('#setting-change-admin-password-confirm1');//ConfrimPassword1 ID
				var passwordhint = $('#setting-change-admin-password-popup-hint');//PasswordHint ID
				var passwordhintanswer = $('#setting-change-admin-password-popup-hint-answer');//PasswordHintAnswer ID
				var newpassword2 = $('#setting-change-admin-password-newpassword2');//NewPassword2 ID
				var confirmpassword2 = $('#setting-change-admin-password-confirm2');//ConfirmPasword2
				if(newpassword1.is(':hidden') && confirmpassword1.is(':visible')){//設定value
					newpassword1.val('');
					newpassword1.val(newpassword2.val());
				}
				if(newpassword1.is(':visible') && confirmpassword1.is(':hidden')){//設定value
					confirmpassword1.val('');
					confirmpassword1.val(confirmpassword2.val());
				}
				if(newpassword1.is(':hidden') && confirmpassword1.is(':hidden')){//設定value
					newpassword1.val('');
					confirmpassword1.val('');
					newpassword1.val(newpassword2.val());
					confirmpassword1.val(confirmpassword2.val());
				}
				if(oldpassword.val() == '' || newpassword1.val() =='' || confirmpassword1.val() == '' || passwordhint.val() == '' || passwordhintanswer.val() == ''){//檢核
					var word1 = multiLang.search('Incomplete response');
					t4u.showMessage(word1);
				}
				else if((newpassword1.val().length < 8) || (confirmpassword1.val().length < 8)){//檢核
					var word2 = multiLang.search('New password field is at least 8 characters');
					t4u.showMessage(word2);
				}
				else if(newpassword1.val() != confirmpassword1.val()){//檢核
					var word3 = multiLang.search('New password and old password do not match');
					t4u.showMessage(word3);
				}
				else{//檢核,成功就儲存
					t4u.nimbusApi.setting.adminLogin(function(data){
						if(t4u.nimbusApi.getErrorCode(data) == 0){
							t4u.nimbusApi.setting.updateAdminInfo(function(data){
								if(t4u.nimbusApi.getErrorCode(data) == 0){
									t4u.showMessage(multiLang.search('ChangeAdminSuccess'));
								}
								else
									t4u.showMessage(data.nimbus.errmsg);
							},
							function(data){
							},
							{password:newpassword1.val(),hint:passwordhint.val(),hintAnswer:passwordhintanswer.val()},TokenUrl(data.nimbus.token));
							t4u.unblockUI();
							ChangeAdminPasswordSetting.clearData();
						}
						else{
							var word4 = multiLang.search("Old password incorrect. Please try again.");
							t4u.showMessage(word4);
						}
					},function(data){
					},
					{password:oldpassword.val(),unique:NewGuid()});
				}
			});
		},
		clearData:function(){
			$('#setting-change-admin-password-popup-window .removeall').val('');	//消除的Class ID	
			ChangeAdminPasswordSetting.showFirstPassword = false;
			ChangeAdminPasswordSetting.showSecondPassword = false;
			$('#setting-change-admin-password-checkbox1').css({'background':' url(data/images/icons/icon_grayCheckBox.png) no-repeat 5px 5px '});
			$('#setting-change-admin-password-checkbox2').css({'background':' url(data/images/icons/icon_grayCheckBox.png) no-repeat 5px 5px '});
		}
	}; 
	/*** 
		Author: Ryan
		Desc: Handle Connect to Internet
	*/
	var ConnectToInternetSetting ={
		AParray: {},//用來存放回傳的AP資訊
		AParrayIndex: -1,//存放選擇到的AP Array的 index
		//初始化物件
		setting_internetAP:false,
		init: function(){
			ConnectToInternetSetting.bind();
			
			t4u.nimbusApi.setting.getUseServices(function(data){
				if(t4u.nimbusApi.getErrorCode(data) == 0)
				{
					if(data.nimbus.MSCMode) {
						$("#setting-msc-on").addClass(cssRadioButtonSelected);
						$("#setting-msc-off").removeClass(cssRadioButtonSelected);
						GetSetting.setting_mscmode = true;

					} else {
						$("#setting-msc-on").removeClass(cssRadioButtonSelected);
						$("#setting-msc-off").addClass(cssRadioButtonSelected);
						GetSetting.setting_mscmode = false;
					}
					
					ConnectToInternetSetting.setting_internetAP = (typeof(data.nimbus.internetAP) == 'object'?data.nimbus.internetAP['text']:data.nimbus.internetAP)?true:false;
					if(ConnectToInternetSetting.setting_internetAP){
						$('.wifi_setting').show();
						$("#wifi-on").addClass(cssRadioButtonSelected);
						$("#wifi-off").removeClass(cssRadioButtonSelected);
					}else{
						$('.wifi_setting').hide();
						$("#wifi-on").removeClass(cssRadioButtonSelected);
						$("#wifi-off").addClass(cssRadioButtonSelected);						
					}
				}
			},
			function(data){
			});
		},
		//清除畫面上的資料
		clear: function(){
			$('#setting-connect-to-internet-popup-password-window-password').val('');//清空Password的值
			$('.overview').empty();//清空列表的值
			$('#setting-connect-to-internet-popup-ssid').val('');//清空列表的SSID的值
			ConnectToInternetSetting.AParrayIndex = -1;			
		},
		showPasswordDialog: function(){
			var password1 = $('#setting-connect-to-internet-popup-password-window-password');//Password1
			var password2 = $('#setting-connect-to-internet-popup-password-window-password2');//Password2
			password1.show();
			password2.hide();
			var word = multiLang.search('show characters');
			$('#setting-to-internet-popup-window-checkbox').text(word);
			$('#setting-to-internet-popup-window-checkbox').css({'background':' url(/data/images/icons/icon_grayCheckBox.png) no-repeat 5px 5px '});
			t4u.blockUI($('#setting-connect-to-internet-popup-password-window'));//跳出password視窗的ID
			//show密碼
			$('#setting-to-internet-popup-window-checkbox').toggle(function(){ //CheckBox1 ID(span)
				$(this).css({'background':' url(/data/images/icons/icon_grayCheckBoxChecked.png) no-repeat 5px 5px'});//打勾圖出現
				password1.hide();
				password2.val(password1.val());
				password2.show();
			},function(){
				$(this).css({'background':' url(/data/images/icons/icon_grayCheckBox.png) no-repeat 5px 5px '});//消失
				password2.hide();
				password1.val(password2.val());
				password1.show();
								  
			});
		},
		loadDataFromServer: function(loading){
			var timer = $('.addDynamicLoading2'),
				viewport = $('.viewport');
			if(loading)
			{
				viewport.hide();
				timer.show();
			}
			else
			{
				viewport.show();
				timer.hide();
			}			
		},
		//繫結畫面上的按鈕
		bind: function(){
			//跳出AP列表
			// wap security selection
				$('#networkSecurity ul li').click(function() {
					$('#networkSecurity h4').text($(this).text());
				});
			
			//點選反應msc on or off
			$('#wifi-on').click(function(){				
					$("#wifi-on").addClass(cssRadioButtonSelected);
					$("#wifi-off").removeClass(cssRadioButtonSelected);
					t4u.blockUIMsg();
					t4u.nimbusApi.setting.setUseServices(function(data){
						t4u.unblockUI();
						if(t4u.nimbusApi.getErrorCode(data) == 0){	
							t4u.showMessage(multiLang.search('WifiOn'));
							$('.wifi_setting').show();
						}
						else
						{
							t4u.showMessage(multiLang.search('WifiFailed'));
						}
						}, function(data){t4u.unblockUI();}, {MSCMode: GetSetting.setting_mscmode,DNLA: true, Bonjour: true, FTP: true, internetAP: true }, xmlUrl+"?token="+token);
						ConnectToInternetSetting.setting_internetAP = true;
				});			
				
			$('#wifi-off').click(function(){
					$("#wifi-off").addClass(cssRadioButtonSelected);
					$("#wifi-on").removeClass(cssRadioButtonSelected);
					t4u.blockUIMsg();
					t4u.nimbusApi.setting.setUseServices(function(data){
						t4u.unblockUI();
						if(t4u.nimbusApi.getErrorCode(data) == 0){	
							t4u.showMessage(multiLang.search('WifiOff'));
							$('.wifi_setting').hide();
						}
						else
						{
							t4u.showMessage(multiLang.search('WifiFailed'));
						}
					}, function(data){t4u.unblockUI();}, {MSCMode: GetSetting.setting_mscmode,DNLA: true, Bonjour: true, FTP: true, internetAP: false }, xmlUrl+"?token="+token);
					ConnectToInternetSetting.setting_internetAP = false;
				});	
			
			$('#setting-connect-to-internet-popup').click(function(){//跳出視窗的ID
				var extensions = new Array("WEP","WPA"),//安全協定,可自行增加
					security_type =false;

				//如果現在網路沒有開，就不做底下的動作	
				if(ConnectToInternetSetting.setting_internetAP)
				{	
				ConnectToInternetSetting.loadDataFromServer(true);
				t4u.bgProcess=false;
				ConnectToInternetSetting.clear();
				$('#networkSecurity h4').text('NONE');
				$('#networkSecurity ul li').removeClass('dropdownSelected');
				
				//取得連線的AP資訊
				t4u.nimbusApi.setting.getConnectedAPInfo(function(data){
					var selectedSSID;
					if(t4u.nimbusApi.getErrorCode(data) == 0 && data.nimbus.ap.status == 'connected')
					{
						selectedSSID = data.nimbus.ap.ssid;
					}
					t4u.nimbusApi.setting.getAPList(function(data){
						if(t4u.nimbusApi.getErrorCode(data) != 0)
						{
							t4u.showMessage(multiLang.search(data.nimbus.errmsg));
							ConnectToInternetSetting.loadDataFromServer(false);
							return;
						}
						if(data.nimbus.ap instanceof Array) {
							ConnectToInternetSetting.AParray = data.nimbus.ap;
						} 
						else {
							ConnectToInternetSetting.AParray = new Array;
							ConnectToInternetSetting.AParray[0] = data.nimbus.ap;
						}
						//wifiprotocol = data.nimbus.ap.security;//設定安全協定
						if(typeof(ConnectToInternetSetting.AParray) != 'undefined') {
							for (var i = 0; i < extensions.length; i++){//比對安全協定,回傳true,false
								if(ConnectToInternetSetting.AParray.security == extensions){
									security_type = ture;
								}
							}
							$('.overview').empty();
							for(var i = 0; i < ConnectToInternetSetting.AParray.length; i++) {
								var obj = ConnectToInternetSetting.AParray[i];
								if(typeof obj == 'undefined')
									continue;
								if(obj.snr <= 35)
									snr_code = '';
								else if(35 < obj.snr && obj.snr <= 70)
									snr_code = '2';
								else
									snr_code = '3';
								$('.overview').append('<div><a href="javascript:void(0);" class="ap_list where ' + (selectedSSID == obj.ssid ? 'ap_list_check' : '') + '">'+obj.ssid+'</a><b class="sign'+ snr_code +'" id='+obj.snr+'></b>'+(obj.security != 'none'?'<b class="locked"></b>':'')+'</div>');

							}
						}
						$('.ap_list').click(function(){
							$('.ap_list').removeClass('ap_list_check');
							$(this).addClass('ap_list_check');
							ConnectToInternetSetting.AParrayIndex = $('.ap_list').index($(this));//設定勾選的的值
						   
							var _wasConnected = false,
								_security = 'none',
								_name;
								
							if(ConnectToInternetSetting.AParrayIndex >= 0) {
								if(ConnectToInternetSetting.AParray instanceof Array) {
									_wasConnected = ConnectToInternetSetting.AParray[ConnectToInternetSetting.AParrayIndex].wasConnected; 
									_security = ConnectToInternetSetting.AParray[ConnectToInternetSetting.AParrayIndex].security;
									_name = ConnectToInternetSetting.AParray[ConnectToInternetSetting.AParrayIndex].ssid;
								} else {
									_wasConnected = ConnectToInternetSetting.AParray.wasConnected;
									_security = ConnectToInternetSetting.AParray.security;
									_name = ConnectToInternetSetting.AParray.ssid;
								}
							}
							
							_security = _security.toLowerCase();						
							//如果已經連線過								
							if(_wasConnected)
							{
								t4u.blockUIMsg();
								t4u.nimbusApi.setting.reConnectAP(function(data){
									t4u.unblockUI();
									if(t4u.nimbusApi.getErrorCode(data) == 0)
									{
										t4u.showMessage(multiLang.search('ConnectSuccessfully'));
									}
									else
									{						
										t4u.showMessage(multiLang.search('ConnectFailed'));
									}
									ConnectToInternetSetting.clear();
								},
								function(data){
								},
								{ name: _name}, url);
							}
							else if(_security == 'none')//不用安全性，直接連線
							{
								t4u.blockUIMsg();
								t4u.nimbusApi.setting.connectAP(function(data){
				
									if(t4u.nimbusApi.getErrorCode(data) == 0)
									{
										t4u.unblockUI();
										t4u.showMessage(multiLang.search('ConnectSuccessfully'));
									}
									else
									{						
										t4u.unblockUI();
										t4u.showMessage(multiLang.search('ConnectFailed'));
									}	
									ConnectToInternetSetting.clear();
								},
								function(data){
								},
								{ name: _name, password: '', security: _security}, url);
							}
							else
							{	
								//根據不同的安全性提示密碼長度
								var word;
								if(_security == 'wep')
								{
									word=multiLang.search('Password must be 5 or 13 characters');
								}
								else if(_security == 'wpa' || _security == 'wpa2')
								{
									word=multiLang.search('Password must have at least 8-63 characters');
								}
								$('#setting-connect-to-internet-popup-password-window-pwlength').text(word);
								ConnectToInternetSetting.showPasswordDialog();
							}						
						});						
						ConnectToInternetSetting.loadDataFromServer(false);						
					},
					function(data){
						ConnectToInternetSetting.loadDataFromServer(false);
					},
					url);
				}, function(data){
					ConnectToInternetSetting.loadDataFromServer(false);
				},
				url);
				t4u.bgProcess=true;
				}
				else
					$('.wifi_setting').hide();
				t4u.blockUI($('#setting-connect-to-internet-popup-window'));//開啟的DIV ID
			});
			
			//其他的SSID被輸入時
			$('#setting-connect-to-internet-popup-ssid').keyup(function(){
				$('#AP1').css({'background':''});//回復初始值
				ConnectToInternetSetting.AParrayIndex = -1;//回復初始值
			});
			
			//按下OK並檢核有無選取
			$('#setting-connect-to-internet-popup-OK').click(function(){//wifi列表的OK按鈕
				var APtext = $('#setting-connect-to-internet-popup-ssid');//下方SSID的值
				if(APtext.val() ==''){
					var word = multiLang.search('Select a WiFi network');
					t4u.showMessage(word);
				} 
				else
				{	
					var _security= $.trim($('#networkSecurity h4').text()).toLowerCase();
					if(_security == 'none')
					{			
						var _name = APtext.val();
						t4u.nimbusApi.setting.connectAP(function(data){				
							if(t4u.nimbusApi.getErrorCode(data) == 0)
							{
								t4u.unblockUI();
								t4u.showMessage(multiLang.search('ConnectSuccessfully'));
							}
							else
							{						
								t4u.unblockUI();
								t4u.showMessage(multiLang.search('ConnectFailed'));
							}	
							ConnectToInternetSetting.clear();
						},
						function(data){
						},
						{ name: _name, password: '', security: _security}, url);	
					}
					else
					{
						//根據不同的安全性提示密碼長度
						var word;
						if(_security == 'wep')
						{
							word=multiLang.search('Password must be 5 or 13 characters');
						}
						else if(_security == 'wpa' || _security == 'wpa2')
						{
							word=multiLang.search('Password must have at least 8-63 characters');
						}
						$('#setting-connect-to-internet-popup-password-window-pwlength').text(word);
						ConnectToInternetSetting.showPasswordDialog();						
					}				
				}

			});
			
			//密碼輸入完畢之後
			$('#setting-connect-to-internet-popup-password-window-OK').click(function(){
				//檢查密碼規則是否正確
				var _name = '',
					_security = 'none',
					_wasConnected = false,
					_password = $.trim($('#setting-connect-to-internet-popup-password-window-password').val());
				if(_password == '')
					_password = $.trim($('#setting-connect-to-internet-popup-password-window-password2').val());
				ConnectToInternetSetting.AParrayIndex = $('.ap_list').index($('.ap_list_check'));//設定勾選的的值
				if(ConnectToInternetSetting.AParrayIndex >= 0) {
					if(ConnectToInternetSetting.AParray instanceof Array) {
						_name = ConnectToInternetSetting.AParray[ConnectToInternetSetting.AParrayIndex].ssid;
						_security = ConnectToInternetSetting.AParray[ConnectToInternetSetting.AParrayIndex].security;
						_wasConnected = ConnectToInternetSetting.AParray[ConnectToInternetSetting.AParrayIndex].wasConnected; 
					} else {
						_name = ConnectToInternetSetting.AParray.ssid;
						_security = ConnectToInternetSetting.AParray.security;
						_wasConnected = ConnectToInternetSetting.AParray.wasConnected;
					}
				} else {
					_name = $.trim($('#setting-connect-to-internet-popup-ssid').val());
					_security= $.trim($('#networkSecurity h4').text());
				}
				_security = _security.toLowerCase();
				
				if(_password == '')
				{
					t4u.showMessage(multiLang.search('Password field is required'));
					return;
				}
				else if(_security == 'wep' && _password.length != 5 && _password.length != 13)
				{
					var word=multiLang.search('Password must be 5 or 13 characters');
					t4u.showMessage(word);
					return;
				}
				else if((_security == 'wpa' || _security == 'wpa2') && (_password.length<8 || _password.length>63 ))
				{
					var word=multiLang.search('Password must have at least 8-63 characters');
					t4u.showMessage(word);
					return;
				}
				
				//t4u.unblockUI();
				t4u.blockUIMsg();				
				//t4u.bgProcess = false;				
				t4u.nimbusApi.setting.connectAP(function(data){
					t4u.unblockUI();
					if(t4u.nimbusApi.getErrorCode(data) == 0)
					{						
						t4u.showMessage(multiLang.search('ConnectSuccessfully'));
					}
					else
					{						
						t4u.showMessage(multiLang.search('ConnectFailed'));
					}	
					ConnectToInternetSetting.clear();
				},
				function(data){
				},
				{ name: _name, password: _password, security: _security}, url);
			});
			
			//password視窗的Cancel
			$('#setting-connect-to-internet-popup-password-window-cancel').click(function(){//password視窗的Cancel ID
				t4u.unblockUI();
				ConnectToInternetSetting.clear();
			});
			
			//回到Setting畫面
			$('#setting-connect-to-internet-popup-back').click(function(){//AP列表的Back按鍵的ID
				t4u.unblockUI();
				ConnectToInternetSetting.clear();
			});
		}
	};


	/***
		Author: Richard
		Desc: Handle Subscribe
	*/
	var GetSetting = {
		setting_mscmode: false,		
		init: function(){
			t4u.nimbusApi.setting.getSSID(function(data){
				if(t4u.nimbusApi.getErrorCode(data) == 0)
					$('#ssid_name').val(data.nimbus.name);
					
					if('anywpa2' == data.nimbus.security)
					{
						$('#setting-security-select-wap').trigger('click');
						$('#setting-security-select-wap').trigger('click');
					}
					else if('wep' == data.nimbus.security)
					{
						$('#setting-security-select-wepkey').trigger('click');
						$('#setting-security-select-wepkey').trigger('click');
					}
			},
			function(data){
			});
			t4u.nimbusApi.setting.getUseServices(function(data){
				if(t4u.nimbusApi.getErrorCode(data) == 0)
				{
					if(data.nimbus.MSCMode) {
						$("#setting-msc-on").addClass(cssRadioButtonSelected);
						$("#setting-msc-off").removeClass(cssRadioButtonSelected);
						GetSetting.setting_mscmode = true;

					} else {
						$("#setting-msc-on").removeClass(cssRadioButtonSelected);
						$("#setting-msc-off").addClass(cssRadioButtonSelected);
						GetSetting.setting_mscmode = false;
					}
				}
			},
			function(data){
			});
		}
	};

	window.mysetting = mainsetting;
	// Ryan End	
	
	//Logout
	//Steven
	var LogoutSetting = {
		init: function()
		{
			this.bind();
		},
		bind: function()
		{
			//設定click事件
			$('#lblLogout').click(function(){
				t4u.bgProcess=false;
				t4u.blockUI($('#div-logout-admin')); 
				t4u.bgProcess=true;
			});
			//取消
			$('#btnLogoutCancel').click(function(){
				t4u.unblockUI();	 
			});
			//確認
			$('#btnLogoutOK').click(function(){
				t4u.SS.updateUI(false);	
				t4u.unblockUI();				
			});			
		}		
	};
	
	//Logout
	//Steven
	var AboutSetting = {
		init: function()
		{
			this.bind();
		},
		bind: function()
		{
			//設定click事件
			$('#setting-about').click(function(){
				t4u.blockUI($('#div-about')); 
			});
			$('#div-about-back').click(function(){
				t4u.unblockUI();	
			});
		}		
	};
	
	var LanguageSetting = {
		init: function(){
			this.bind();
		},		
		bind: function(){
			$('#divLanguage').click(function(){
				$('#div-language-dialog a').removeClass('language_check');
				$('#div-language-dialog .language-list a[language*="' + multiLang.currentLang + '"]').addClass('language_check');
				LanguageSetting.reloadLanguage(multiLang.currentLang);
				t4u.blockUI($('#div-language-dialog')); 
			});
			
			$('#btnLanguageCancel').click(function(){
				t4u.unblockUI();
			});
			
			$('#btnLanguageOK').click(function(){	
				var lang = $('#div-language-dialog .language_check').attr('language');
				LanguageSetting.reloadLanguage(lang);
				switch(lang)
				{
					case 'en':
						$('#gettingStartedImg').addClass('gettingStarted');
						break;
					case 'fr':
						$('#gettingStartedImg').addClass('gettingStartedFra');
						break;
					case 'de':
						$('#gettingStartedImg').addClass('gettingStartedDeu');
						break;
					case 'it':
						$('#gettingStartedImg').addClass('gettingStartedIta');
						break;
					case 'es':
						$('#gettingStartedImg').addClass('gettingStartedEsp');
						break;
				}
				
				t4u.unblockUI();				
			});
			
			$('#div-language-dialog a').click(function(){					
				$('#div-language-dialog a').removeClass('language_check');
				$(this).addClass('language_check');								
			});
		},
		reloadLanguage: function(language)
		{
			$('#gettingStartedImg').removeClass();
			
			multiLang.manual(language);
			t4u.SS.ChangeLang();
			$('#btnUpload').hide();
			$('#btnNew').hide();
			$('#btnDownload').hide();
			$('#btnDel').hide();
			$('.noMusicInfo').hide();
		}		
	};
})(window);
