function alignFrames2()
% function alignFrames2()
%
% Can we collect multiple frames with short exposure times and then align
% them to minimize blur and maintain dynamic range? How fast can the
% alignment be done?
%
% Could a DNN be designed to do this alignment for a
% fixed number of frames but vehicles going at various speeds? 


%dirRoot = './captures1';
%dirRoot = './captures_mono8';
%dirRoot = './captures_cloudy_mono12_1000us';
dirRoot = './captures_sunny_mono12_1000us';
%dirRoot = '/home/mroos/scratch/';

%d = dir([dirRoot '/*.png']);
d = dir([dirRoot '/*.tiff']);
%d = dir([dirRoot '/*.bmp']);


nPics = length(d);
nPicsPerGroup = 6;
nGroups = nPics/nPicsPerGroup;
maxLag = 300;

clrs = jet(nPicsPerGroup-1);

[szY szX] = size(imread(fullfile(dirRoot,d(1).name)));

for iGroup = 2:nGroups
   
   fn = 1;
   
   im = nan([szY szX nPicsPerGroup]);

   figure(fn); clf; fn=fn+1; colormap gray;
   for iPic = 1:nPicsPerGroup
      im(:,:,iPic) = double(imread(fullfile(dirRoot,d((iGroup-1)*nPicsPerGroup+iPic).name)));      
   end
   mx = max(im(:));
   for iPic = 1:nPicsPerGroup
      subplot(3,2,iPic);
      imagesc(im(:,:,iPic));
      caxis([0 mx]); colorbar;
   end
   imtrue = im;

   
   % Apply edge detection filter first, then do cross correlations
   % between edge images...
   nh = 10;
   hFilt = (1:nh)'/nh;
   hFilt = sqrt(hFilt);   % useful?
   hFilt = [hFilt; flipud(-hFilt)];
   for iPic = 1:nPicsPerGroup
      im(:,:,iPic) = abs(fftfilt(hFilt,im(:,:,iPic)')');
   end
   
   
   % Do cross correlations
   figure(fn); clf; fn=fn+1; colormap gray;
   shift = nan(nPicsPerGroup,1);
   shift(1) = 0;
   imshift = zeros(size(im));
   
   h = nan(nPicsPerGroup-1,1);
   for iPic1 = 1:nPicsPerGroup-1
      iPic2 = iPic1+1;
      fprintf('Pics %d and %d\n',iPic1,iPic2);

      ac12 = zeros(szY,2*maxLag+1);
      xc12 = zeros(szY,2*maxLag+1);
      
      % use arrayfun to parallelize below?
      imdiff = abs(im(:,:,iPic1)-im(:,:,iPic2));
      for iY = 1:szY
         %fprintf('%d\n',iY);
         %xc_gainMask(iY,:) = normDiffCorr(cim1(iY,:),cim2(iY,:),[maxLag maxLag]);
         ac12(iY,:) = xcorr((im(iY,:,iPic1)+im(iY,:,iPic2))/2,(im(iY,:,iPic1)+im(iY,:,iPic2))/2,maxLag,'unbiased');
         xc12(iY,:) = xcorr(im(iY,:,iPic1),im(iY,:,iPic2),maxLag,'unbiased');
      end
      xc = sum(xc12)-sum(ac12);
      
      [~,ixMax] = max(xc);
      shift(iPic2) = ixMax - (maxLag+1);
      h(iPic1) = line(-maxLag:maxLag,xc,'Color',clrs(iPic1,:));
      line(ixMax-(maxLag+1),xc(ixMax),'Marker','o');
   end
   legend(h,arrayfun(@num2str,1:nPicsPerGroup-1,'UniformOutput',false));
   box on; gz;
   
   figure(fn); clf; fn=fn+1; colormap gray;
   for iPic = 1:nPicsPerGroup-1
      %imshift(:,:,iPic) = circshift(im(:,:,iPic),[0 shift(iPic)]);
      %imshift(:,:,iPic) = padshift(im(:,:,iPic),shift(iPic));
      x = im(:,:,iPic) + padshift(im(:,:,iPic+1),shift(iPic+1));
      subplot(3,2,iPic);
      imagesc(x);
   end
   
   im = imtrue;

   shift = cumsum(shift)-shift(3);
   for iPic = 1:nPicsPerGroup
      %imshift(:,:,iPic) = circshift(im(:,:,iPic),[0 shift(iPic)]);
      imshift(:,:,iPic) = padshift(im(:,:,iPic),shift(iPic));
   end
   
   figure(fn); clf; fn=fn+1; colormap gray;
   for iPic = 1:nPicsPerGroup-1
      subplot(3,2,iPic);
      imagesc(imshift(:,:,iPic)+imshift(:,:,iPic+1));
   end
   
   imsum = sum(imshift,3);
   subplot(3,2,6);
   imagesc(imsum);

   pause      
end
end


%% SUBFUNCTIONS
function c = normDiffCorr(a,b,maxLags)
% c = SUM[ (a(x)-b(x-x0)).^2 / (a(x)+b(x-x0)) ]
% maxLags = [max leftward lag of b; max rightward lag of b]

   % For now, force length of a and b to be same length
   L = length(a);
   if length(b)~=L
      error('a and b must be same length.');
   end
   
   D = bsxfun(@minus,a(:),b(:)').^2;
   S = bsxfun(@plus,a(:),b(:)').^2;

   N = sum(maxLags)+1;
   num = nan(N,1);
   den = nan(N,1);
   for lag = -maxLags(1):maxLags(2)
      num(lag+maxLags(1)+1) = sum(diag(D,lag));
      den(lag+maxLags(1)+1) = sum(diag(S,lag));
   end
   c = flipud(num./den);
end

function Y = padshift(X,K)
% Like 2-D circshift except not circular. Padded with zeros.
   Y = zeros(size(X));
   if K>=0
      Y(:,K+1:end) = X(:,1:end-K);
   else
      Y(:,1:end+K) = X(:,-K+1:end);
   end
end

