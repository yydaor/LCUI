if (typeof $ != "undefined") {
  $(function() {
    if (Hp && Hp.FlyoutMenu) {
      var MenuClick;
      var MenuClose;
      if ($.bi && $.bi.dataRetrievers.structure) {
        MenuClick = function(e) {

          var element = $(e.target);
          var elements = $(e.target).parents('.msMnu_Item').find('.msPopup_Position a');
          /* Merge Params from the additionalParams*/
          var mergedbiData = $.bi.getAttrData(elements);
          var customParams = {
            title: $.trim($(element).text())
          };
          try {
            $.extend(customParams, $.bi.baseData(), $.bi.dataRetrievers.structure.getData(element));
          } catch(e) {}

          var openCustomParam = $.bi.getAttrData(element.find('.hpBi_Custom .hpBi_Action-open'));

          /* Open Bi Event
                            InteractionType=9;Initial=0;Title=BladeItem;ParentTypeStructure;ParentIndexStructure(same)*/
          var openBIParams = $.extend({},
          customParams, openCustomParam, {
            initial: ($(e.target).data('bimenuopen') !== undefined ? $(e.target).data('bimenuopen') : 0),
            interactiontype: 9,
            index: customParams['parentindexstructure'],
            parentindexstructure: '',
            '-bi-description-ignore-': 'menu-open'
          });

          $(e.target).data('bimenuopen', 1);
          /* Blade Index is Index*/
          $.bi.queue.push(openBIParams);

          /* View Bi Event 
                            InteractionType=1;Initial=0;TriggerType=9;Title='';ParentTitleStructure=BladeItemTitle;Type=SubComponentType;ParentTypeStructure;ParentIndexStructure(same) */
          var viewBIParams = $.extend({},
          customParams, mergedbiData, {
            initial: $(e.target).data('bimenuview') !== undefined ? $(e.target).data('bimenuview') : 0,
            interactiontype: 1,
            triggertype: 9,
            parenttitlestructure: $.trim($(element).text()),
            title: '',
            type: $(e.target).parents('.hpBld_Item').find('[bi\\:type]').first().attr('bi:type')
            /* look for first child bi:type */
            ,
            '-bi-description-ignore-': 'menu-view'
          });
          $(e.target).data('bimenuview', 1);
          $.bi.queue.push(viewBIParams);
        };
        MenuClose = function($flyoutLink) {
          /*  Menu Close Function InteractionType = 10;Initial = 0/1;Title;ParentTypeStructure;IndexStructure; Remove : ParentTitleStructure */
          var customParams = {};

          $.extend(customParams, $.bi.getAttrData($flyoutLink.find('.hpBi_Custom .hpBi_Action-close')));

          $.extend(customParams, $.bi.baseData(), $.bi.dataRetrievers.structure.getIndexGridTypeStructure($flyoutLink));
          $.extend(customParams, {
            initial: $flyoutLink.data('bimenuclose') !== undefined ? $flyoutLink.data('bimenuclose') : 0,
            interactiontype: 10,
            title: $.trim($flyoutLink.text()),
            index: customParams['parentindexstructure'],
            parentindexstructure: '',
            '-bi-description-ignore-': 'menu-close'
          });

          $flyoutLink.data('bimenuclose', 1);
          $.bi.queue.push(customParams);
          /* Close Record*/

        };
      }
      var menu = new Hp.FlyoutMenu('ctl00_ctl11', "msMnu", "msMnu_Item", "msPopup_Link", "msMnu-link", "msPopup", "msFlyout_Region", MenuClick, MenuClose, "col",
      function(menu) {
        return menu.width();
      },
      function(menu) {
        return menu.position().left;
      });
    }

  });
}