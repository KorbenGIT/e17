#------------------------------ EWL example program ------------------------------------------------
# This is a test program that shows an example of the Ewl bindings usage.
# It is not guaranteed to always work after API changes (it's not autogenerated like the bindings)
#---------------------------------------------------------------------------------------------------

require 'dl/ffcall-callback'
require 'ruby-efl/ewl/ruby-ewl'
#require '../ruby-ewl.rb'       # if you want to use the local version

# CClass.debug = true           # uncomment this on when you want to see lots of debug info

include Ewl

class TestWindow
	PACKAGE_DATA_DIR = '/opt/e17/share/ewl'
	XDATA = [
		PACKAGE_DATA_DIR + "/images/e-logo.png",
		PACKAGE_DATA_DIR + "/images/Draw.png",
		PACKAGE_DATA_DIR + "/images/End.png",
		PACKAGE_DATA_DIR + "/images/World.png"
	]

	def initialize

		labeltext = "This is a Label. Type something below and hit <enter>."
	
		@win = Window.new
		@win.title = "EWL Window"
		@win.name = "EWL_WINDOW"
		@win.class = "EWLWindow"
		@win.size_request(800, 600)
		@win.fill_policy = Ewl::FLAG_FILL_ALL 

		glob = HBox.new
		@win << glob
		glob.homogeneous = false
		glob.fill_policy = Ewl::FLAG_FILL_ALL
		glob.show

		border = Border.new
		glob << border
		border.homogeneous = false
		border.fill_policy = Ewl::FLAG_FILL_FILL
		border.alignment = Ewl::FLAG_ALIGN_CENTER
		border.text = "This is a Border"
		border.show

		label = Text.new
		border << label
		label.alignment = Ewl::FLAG_ALIGN_CENTER
		label.color_set(255, 0, 0, 255)   
		label.text = labeltext
		label.show
        
		txtarea = Entry.new
		border << txtarea
		txtarea.alignment = Ewl::FLAG_ALIGN_CENTER
		txtarea.padding_set(5, 5, 5, 0)
		txtarea.color_set(0, 0, 255, 255)
		txtarea.text = "This is an Entry"
		txtarea.show

		@butt = Button.new
		border << @butt
		@butt.alignment = Ewl::FLAG_ALIGN_CENTER
		@butt.object_fill_policy = Ewl::FLAG_FILL_VSHRINK | Ewl::FLAG_FILL_HFILL
		@butt.label = "Press me !"
		@butt.show

		@trig = Button.new
		border << @trig
		@trig.alignment = Ewl::FLAG_ALIGN_CENTER
		@trig.object_fill_policy = Ewl::FLAG_FILL_VSHRINK | Ewl::FLAG_FILL_HFILL
		@trig.label = "Selection info"
		@trig.show
		
		spane = Scrollpane.new
		border << spane
		spane.show

		txtread = Text.new
		txtread.alignment = Ewl::FLAG_ALIGN_CENTER
		txtread.padding_set(5, 5, 5, 0)
		txtread.color_set(0, 0, 255, 255)
		txtread.selectable = true
		txtread.show
		spane << txtread
		txtread << Ewl::RealizeCallback.new { txtread.text = IO.read(File.dirname(File.expand_path(__FILE__)) + '/ewl-example.rb') }

		midcol = VBox.new
		midcol.fill_policy = Ewl::FLAG_FILL_FILL
		midcol.show
		glob << midcol

		sep = VSeparator.new
		sep.show
		glob << sep

		rxcol = VBox.new
		rxcol.fill_policy = Ewl::FLAG_FILL_FILL
		rxcol.show
		glob << rxcol
		
	 	cpick = ColorPicker.new
		cpick.alignment = Ewl::FLAG_ALIGN_TOP
		cpick.has_alpha = true
		cpick.show
		midcol << cpick
		cpick << Ewl::ClickedCallback.new { 
			cpick.current_rgb(0,0,0) { |ret,r,g,b| 
				label.color_set(r, g, b, 255)
				#label.text = labeltext
			}
		}

		spec = Spectrum.new
		spec.alignment = Ewl::FLAG_ALIGN_TOP
		spec.show
		midcol << spec
		
		combo = setup_combo()
		midcol << combo

		passw = Password.new
		passw.alignment = Ewl::FLAG_ALIGN_CENTER
		passw.obscure = '*'
		passw.text = "You can't read this"
		passw.show
		midcol << passw
		
		seekr = Seeker.new
		seekr.alignment = Ewl::FLAG_ALIGN_TOP
		seekr.min_val = 0.0
        seekr.max_val = 1.0
		seekr.step = 0.25
		seekr.value = 0.5
		seekr.show
		midcol << seekr

		sbar = Scrollbar.new
		sbar.alignment = Ewl::FLAG_ALIGN_TOP
		sbar.orientation = Ewl::ORIENTATION_HORIZONTAL
		sbar.step = 0.25
		sbar.value = 0.5
		sbar.show
		midcol << sbar

		spin = Spinner.new
		spin.alignment = Ewl::FLAG_ALIGN_TOP
		spin.min_val = 0.0
		spin.max_val = 1.0
		spin.step = 0.25
		spin.value = 0.5
		spin.show
		midcol << spin

		spane2 = Scrollpane.new
		spane2.show
		midcol << spane2

		image = Image.new
		image.show
		image.scale_set(0.5, 0.5)
		spane2 << image
		image << Ewl::RealizeCallback.new { image.file_path = XDATA[0] }
		@image = image

		sep = HSeparator.new
		midcol << sep
		sep.show

		bits = HBox.new
		bits.show
		#midcol << bits

		checkb = CheckButton.new
		checkb.checked = true
		checkb.label = "I'm a checkbutton"
		checkb.show
		bits << checkb
		checkb << Ewl::ValueChangedCallback.new {
			is_on = checkb.is_checked?
			puts "[checkbutton] I am now #{(is_on)?"checked":"unchecked"}"
		}

		radio1 = RadioButton.new
		#radio1.checked = true
		radio1.label = "On"
		radio1.show

		radio2 = RadioButton.new
		radio2.checked = true
		radio2.label = "Off"
		radio2.show

		bits << radio1
		bits << radio2
		radio1.chain = radio2

		check = Check.new
		check.checked = true
		check.show
		bits << check
		check << Ewl::ValueChangedCallback.new {
			is_on = check.is_checked?
			puts "[check] I am now #{(is_on)?"checked":"unchecked"}"
		}

		dpick = DatePicker.new
		dpick.show
		midcol << dpick
		dpick << Ewl::ValueChangedCallback.new {
			puts "[datepicker] text is #{dpick.text}"
		}

		cal = Calendar.new
		cal.fill_policy = Ewl::FLAG_FILL_SHRINK
		cal.show
		rxcol << cal
		cal << Ewl::ValueChangedCallback.new {
			puts "[calendar] ASCII: #{cal.ascii_time} or [#{cal.day}],[#{cal.month}],[#{cal.year}]"
		}

		ico = Icon.new
		ico.type = Ewl::ICON_TYPE_LONG
		ico.image_set(XDATA[1], nil)
		ico.editable = true
		ico.label = "Click on this icon label."
		ico.extended_data = bits  
		ico.constrain = 10
		ico.show
		midcol << ico

		sep = HSeparator.new
		sep.show
		midcol << sep

		# Setup some callbacks
		#

		@win << Ewl::DeleteWindowCallback.new {
			@win.destroy
			EwlBase::main_quit()
		}
		
		cb_spinning_things = Ewl::ValueChangedCallback.new() { |ptr,*|
			master = EObject.wrap(ptr)
			case master
			when seekr; spin.value = sbar.value = seekr.value
			when sbar; spin.value = seekr.value = sbar.value
			when spin; seekr.value = sbar.value = spin.value
			end
			image.scale_set(spin.value, spin.value)
		}
		sbar << cb_spinning_things
		seekr << cb_spinning_things
		spin << cb_spinning_things

		cb_colored_things = lambda { |ptr,*|
			master = EObject.wrap(ptr)
			case master
			when cpick
				cpick.current_rgb(0,0,0) { |ret,r,g,b| spec.rgb_set(r,g,b) }
				spec.mode = cpick.color_mode
			when spec
				spec.rgb(0,0,0) { |ret,r,g,b| cpick.current_rgb_set(r,g,b) }
				cpick.color_mode = spec.mode
			end
		}
		cpick << Ewl::ValueChangedCallback.new(&cb_colored_things)
		spec << Ewl::ValueChangedCallback.new(&cb_colored_things)
		
		cb_tx = lambda { label.text = "Text is: " + txtarea.text }
		txtarea << Ewl::ValueChangedCallback.new(&cb_tx)
		@butt << Ewl::ClickedCallback.new(&cb_tx)

		@trig << Ewl::ClickedCallback.new() { 
			if txtread.has_selection? then
				sel = TextTrigger.wrap(txtread.selection)
				label.text = "Selection from: #{sel.start_pos} length: #{sel.length}"
			else
				label.text = "No selection"
			end
		}
	end
	
	def show; @win.show; end

	# This is converted from the combobox test in the ewl unit tests
	def setup_combo

		model = Model.new
		model.fetch = Ewl::ModelFetchCallback.new { |ptr, row, col|
			d = (row < XDATA.length) ? XDATA[row].to_ptr : nil
			puts "[combo] requested data at #{row}/#{col}. It is \"{d}\""
			d
		}
		
		model.count = Ewl::ModelCountCallback.new { 
			puts "[combo] requested length. It is: #{XDATA.length.to_s}"
			XDATA.length 
		}
		
		view = View.new
		view.constructor = Label.function(:new)
		view.assign = Label.function(:text_set)
		view.header_fetch = Ewl::ViewHeaderFetchCallback.new { |ptr, col|
			puts "requested header fetch of col #{col.to_s}"
			header = Label.new
			header.text = XDATA[0]
			header.show
			header.this #return the internal pointer for the widget. in the future this will be rewrapped automatically 
		}
		
		combo = Combo.new
		combo.name = "combo_label"
		combo << Ewl::ValueChangedCallback.new {
			# d = ewl_combo_data_get(EWL_COMBO(w)); # we don't need this. the data is right here in the block closure
			idx = combo.selected
			if (idx > -1) then puts "Value changed to #{idx.to_s} (#{XDATA[idx]})"
			else puts "Nothing selected." end
			
			@image.file_path = XDATA[idx]
		}
		
		combo.model = model
		combo.view = view
		combo.data = XDATA 
		combo.show
		return combo
	end
end

EwlBase::init(ARGV.length, ARGV)
testwin = TestWindow.new
testwin.show

EwlBase::main()



