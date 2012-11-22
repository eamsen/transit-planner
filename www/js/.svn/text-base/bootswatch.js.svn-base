var menu = 
'<ul class="nav"> \
  <li><a href="/index.html">Overview</a></li> \
  <li class="dropdown"> \
    <a class="dropdown-toggle" data-toggle="dropdown" href="/concepts.html"> \
       Concepts <b class="caret"></b></a> \
	  <ul class="dropdown-menu"> \
	    <li><a href="/concepts.html#public_transport">Public Transport</a></li> \
	    <li class="divider"></li> \
	    <li><a href="/concepts.html#gtfs">GTFS</a></li> \
	    <li><a href="/concepts.html#transfer_patterns">Transfer Patterns</a></li> \
	  </ul> \
	</li> \
  <li class="dropdown"> \
    <a class="dropdown-toggle" data-toggle="dropdown" href="/application.html"> \
       Application <b class="caret"></b></a> \
	  <ul class="dropdown-menu"> \
	    <li><a href="/application.html#architecture">Architecture</a></li> \
	    <li><a href="/application.html#components">Components</a></li> \
	  </ul> \
	</li> \
  <li class="dropdown"> \
    <a class="dropdown-toggle" data-toggle="dropdown" href="/statistics.html"> \
       Statistics <b class="caret"></b></a> \
	  <ul class="dropdown-menu"> \
	    <li><a href="/statistics.html#architecture">Robustness</a></li> \
	    <li><a href="/statistics.html#components">Conclusions</a></li> \
	  </ul> \
  <li class="divider-vertical"></li> \
  <li class="dropdown"> \
    <a class="dropdown-toggle" data-toggle="dropdown" href="/about.html"> \
       About <b class="caret"></b></a> \
	  <ul class="dropdown-menu"> \
	    <li><a href="/about.html#team">Team</a></li> \
	    <li><a href="/about.html#university">University</a></li> \
	  </ul> \
	</li>';

// menu
var menu_ext = menu + '			</ul> \
						<ul class="nav pull-right"> \
							<li><a rel="tooltip" target="_blank" href="http://builtwithbootstrap.com/" title="Showcase of Bootstrap sites & apps" onclick="_gaq.push([\'_trackEvent\', \'click\', \'outbound\', \'builtwithbootstrap\']);">Built With Bootstrap <i class="icon-share-alt icon-white"></i></a></li> \
			  				<li><a rel="tooltip" target="_blank" href="http://wrapbootstrap.com" title="Marketplace for premium Bootstrap templates" onclick="_gaq.push([\'_trackEvent\', \'click\', \'outbound\', \'wrapbootstrap\']);">WrapBootstrap <i class="icon-share-alt icon-white"></i></a></li> \
			  			</ul>';

			$('.navbar .nav-collapse').first().append(menu);

			$('a[rel=tooltip]').tooltip({
				'placement': 'bottom'
			});

var taglines = [];
taglines.push('Free themes for <a target="_blank" href="http://twitter.github.com/bootstrap/">Twitter Bootstrap</a>');
taglines.push('Add color to your <a target="_blank" href="http://twitter.github.com/bootstrap/">Bootstrap</a> site without touching a color picker');
taglines.push('Saving the web from default <a target="_blank" href="http://twitter.github.com/bootstrap/">Bootstrap</a>');

var line = Math.floor((taglines.length) * Math.random());
$('#tagline').html(taglines[line]);

